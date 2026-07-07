from __future__ import annotations

from dataclasses import dataclass
import logging
import time
from threading import Event
from typing import Callable, Optional

import serial
from serial import SerialException

from .protocol import FrameBuffer, ProtocolError, TelemetryPacket, TwinStatePacket


logger = logging.getLogger(__name__)


@dataclass
class SerialConfig:
    port: str
    baudrate: int = 921600
    read_timeout_s: float = 0.1
    reconnect_delay_s: float = 1.0
    max_reconnect_delay_s: float = 5.0


class ReconnectingSerialLink:
    def __init__(self, config: SerialConfig) -> None:
        self._config = config
        self._serial: Optional[serial.Serial] = None
        self._buffer = FrameBuffer()
        self._backoff = config.reconnect_delay_s

    def open(self) -> None:
        self._serial = serial.Serial(
            port=self._config.port,
            baudrate=self._config.baudrate,
            timeout=self._config.read_timeout_s,
        )
        self._backoff = self._config.reconnect_delay_s
        logger.info("opened serial port %s at %d", self._config.port, self._config.baudrate)

    def close(self) -> None:
        if self._serial and self._serial.is_open:
            self._serial.close()
        self._serial = None

    def ensure_open(self, stop_event: Event | None = None) -> serial.Serial:
        if self._serial and self._serial.is_open:
            return self._serial

        while stop_event is None or not stop_event.is_set():
            try:
                self.open()
                assert self._serial is not None
                return self._serial
            except SerialException as exc:
                logger.warning("serial open failed on %s: %s", self._config.port, exc)
                if stop_event is not None and stop_event.wait(self._backoff):
                    break
                self._backoff = min(self._backoff * 2.0, self._config.max_reconnect_delay_s)

        raise SerialException("serial link stopped")

    def write(self, payload: bytes) -> None:
        ser = self.ensure_open()
        try:
            ser.write(payload)
            ser.flush()
        except SerialException:
            self.close()
            raise

    def read_packets(
        self,
        on_packet: Callable[[TelemetryPacket | TwinStatePacket], None],
        stop_event: Event | None = None,
    ) -> None:
        while stop_event is None or not stop_event.is_set():
            ser = self.ensure_open(stop_event)
            try:
                chunk = ser.read(256)
                if not chunk:
                    continue

                # --- DEBUG LOGGING ---
                logger.info("Raw bytes received: %s", chunk.hex())

                for packet in self._buffer.feed(chunk):
                    logger.info("Successfully parsed packet: %s", type(packet).__name__)
                    on_packet(packet)
            except (SerialException, ProtocolError) as exc:
                logger.warning("serial read error on %s: %s", self._config.port, exc)
                self.close()
                if stop_event is not None and stop_event.is_set():
                    break
                time.sleep(self._backoff)
                self._backoff = min(self._backoff * 2.0, self._config.max_reconnect_delay_s)
