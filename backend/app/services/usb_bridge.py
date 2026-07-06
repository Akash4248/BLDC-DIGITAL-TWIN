from __future__ import annotations

import asyncio
import logging
import os
from dataclasses import dataclass
from threading import Event, Thread
from typing import Optional

from backend.app.serial.connection import ReconnectingSerialLink, SerialConfig
from backend.app.serial.protocol import TwinStatePacket, TelemetryPacket
from backend.app.services.simulation_service import SimulationService, SimulationServiceError


logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class UsbBridgeConfig:
    port: str
    baudrate: int = 921600
    read_timeout_s: float = 0.1
    reconnect_delay_s: float = 1.0
    max_reconnect_delay_s: float = 5.0

    @classmethod
    def from_env(cls) -> "UsbBridgeConfig | None":
        port = os.getenv("FOC_USB_SERIAL_PORT", "").strip()
        if not port:
            return None

        baudrate = int(os.getenv("FOC_USB_BAUDRATE", "921600"))
        return cls(port=port, baudrate=baudrate)

    def serial_config(self) -> SerialConfig:
        return SerialConfig(
            port=self.port,
            baudrate=self.baudrate,
            read_timeout_s=self.read_timeout_s,
            reconnect_delay_s=self.reconnect_delay_s,
            max_reconnect_delay_s=self.max_reconnect_delay_s,
        )


class DigitalTwinUsbBridge:
    """
    Optional hardware bridge between the ESP32 FOC controller and the twin engine.

    The bridge is transport-only. It reads telemetry packets from USB, steps the
    simulation deterministically, broadcasts telemetry to dashboard clients, and
    sends feedback packets back to the ESP32.
    """

    def __init__(self, service: SimulationService, config: UsbBridgeConfig) -> None:
        self._service = service
        self._config = config
        self._serial = ReconnectingSerialLink(config.serial_config())
        self._stop_event = Event()
        self._thread: Optional[Thread] = None
        self._loop: Optional[asyncio.AbstractEventLoop] = None

    @property
    def running(self) -> bool:
        thread = self._thread
        return thread is not None and thread.is_alive()

    def start(self, loop: asyncio.AbstractEventLoop) -> None:
        if self.running:
            return

        self._loop = loop
        self._stop_event.clear()
        self._service.start()
        self._thread = Thread(target=self._run, name="usb-bridge", daemon=True)
        self._thread.start()
        logger.info("USB bridge started on %s at %d baud", self._config.port, self._config.baudrate)

    def stop(self) -> None:
        self._stop_event.set()
        self._serial.close()
        thread = self._thread
        if thread and thread.is_alive():
            thread.join(timeout=2.0)
        self._thread = None
        self._service.stop()
        logger.info("USB bridge stopped")

    def _run(self) -> None:
        loop = self._loop
        if loop is None:
            logger.error("USB bridge cannot start without an event loop")
            return

        def handle_packet(packet: TwinStatePacket | TelemetryPacket) -> None:
            if isinstance(packet, TwinStatePacket):
                payload = {
                    "telemetry": {
                        "sequence": packet.sequence,
                        "timestamp_us": 0,
                        "duty_a": 0, "duty_b": 0, "duty_c": 0, "vdc": 0
                    },
                    "current": {
                        "ia": packet.ia,
                        "ib": packet.ib,
                        "ic": packet.ic,
                        "adc_a": 0, "adc_b": 0, "adc_c": 0
                    },
                    "speed": packet.rotor_speed,
                    "torque": 0.0,
                    "rotor_angle": packet.rotor_angle,
                    "hall": {"a": False, "b": False, "c": False},
                    "encoder": {"a": False, "b": False, "index": False},
                    "diagnostics": {
                        "max_exec_time": getattr(packet, "max_exec_time", 0),
                        "missed_deadlines": getattr(packet, "missed_deadlines", 0)
                    }
                }
                # Broadcast directly to websocket clients
                asyncio.run_coroutine_threadsafe(
                    self._service.broadcaster.broadcast_raw(payload), loop
                )
            else:
                # Fallback for telemetry packets if needed
                pass

        try:
            self._serial.read_packets(handle_packet, stop_event=self._stop_event)
        except Exception as exc:
            logger.exception("USB bridge loop terminated unexpectedly: %s", exc)

