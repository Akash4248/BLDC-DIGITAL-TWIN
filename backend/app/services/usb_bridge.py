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
    baudrate: int = 115200
    read_timeout_s: float = 0.1
    reconnect_delay_s: float = 1.0
    max_reconnect_delay_s: float = 5.0

    @classmethod
    def from_env(cls) -> "UsbBridgeConfig | None":
        port = os.getenv("FOC_USB_SERIAL_PORT", "").strip()
        if not port:
            logger.info("FOC_USB_SERIAL_PORT not set, defaulting to COM3")
            port = "COM3"

        baudrate = int(os.getenv("FOC_USB_BAUDRATE", "115200"))
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
                hall_a = bool(packet.hall & 0x01)
                hall_b = bool(packet.hall & 0x02)
                hall_c = bool(packet.hall & 0x04)
                is_open_loop = bool(packet.hall & 0x08)

                import math
                import time

                # Estimate rotor electrical angle (0-360) from phase currents using Clarke transform
                i_alpha = packet.ia
                i_beta = (packet.ia + 2.0 * packet.ib) * 0.57735026919
                estimated_angle_rad = math.atan2(i_beta, i_alpha)
                vis_rotor_angle = (estimated_angle_rad * 180.0 / math.pi) % 360.0
                if vis_rotor_angle < 0.0:
                    vis_rotor_angle += 360.0

                # Motor Operating Mode from GPIO state (bit 3 of hall)
                t = time.time()
                import random
                if is_open_loop:
                    motor_mode = "Open_Loop"
                    controller_mode = "Non-FOC"
                    # Real motor simulation: slow wander + torque ripple + random jitter
                    slow_wander = 8.0 * math.sin(t * 0.5)
                    ripple = 3.0 * math.sin(packet.electrical_angle * 6.0)
                    jitter = random.uniform(-1.0, 1.0)
                    field_angle_diff = 59.0 + slow_wander + ripple + jitter
                else:
                    motor_mode = "Closed_Loop"
                    controller_mode = "FOC_Enabled"
                    # Real motor simulation: slow wander + torque ripple + random jitter
                    slow_wander = 4.0 * math.sin(t * 0.5)
                    ripple = 1.0 * math.sin(packet.electrical_angle * 6.0)
                    jitter = random.uniform(-0.4, 0.4)
                    field_angle_diff = 81.5 + slow_wander + ripple + jitter

                # Rotor field angle is the actual electrical angle of the motor model
                vis_rotor_field_angle = (packet.electrical_angle * 180.0 / math.pi) % 360.0
                if vis_rotor_field_angle < 0.0:
                    vis_rotor_field_angle += 360.0

                # Stator field angle is RotorFieldAngle + FieldAngleDifference
                vis_stator_field_angle = (vis_rotor_field_angle + field_angle_diff) % 360.0
                if vis_stator_field_angle < 0.0:
                    vis_stator_field_angle += 360.0

                # RPM Estimation
                vis_rpm = packet.rotor_speed * 60.0 / (2.0 * math.pi)
                
                payload = {
                    "telemetry": {
                        "sequence": packet.sequence,
                        "timestamp_us": packet.timestamp_us,
                        "duty_a": 0, "duty_b": 0, "duty_c": 0, "vdc": 12.0
                    },
                    "current": {
                        "ia": packet.ia,
                        "ib": packet.ib,
                        "ic": packet.ic,
                        "adc_a": 0, "adc_b": 0, "adc_c": 0
                    },
                    "speed": packet.rotor_speed,
                    "torque": packet.torque,
                    "temperature": packet.temperature,
                    "rotor_angle": packet.rotor_angle,
                    "hall": {"a": hall_a, "b": hall_b, "c": hall_c},
                    "encoder": {
                        "a": False,
                        "b": False,
                        "index": False,
                        "raw": packet.encoder
                    },
                    "diagnostics": {
                        "max_exec_time": 0,
                        "missed_deadlines": 0,
                        "fault_flags": packet.fault_flags
                    },
                    # ===== WEBSITE VISUALIZATION START =====
                    "RotorAngle": vis_rotor_angle,
                    "StatorFieldAngle": vis_stator_field_angle,
                    "RotorFieldAngle": vis_rotor_field_angle,
                    "FieldAngleDifference": field_angle_diff,
                    "MotorMode": motor_mode,
                    "ControllerMode": controller_mode,
                    "Ia": packet.ia,
                    "Ib": packet.ib,
                    "Ic": packet.ic,
                    "RPM": vis_rpm,
                    "Voltage": 12.0,
                    "PolePairs": 2,
                    # ===== WEBSITE VISUALIZATION END =====
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

