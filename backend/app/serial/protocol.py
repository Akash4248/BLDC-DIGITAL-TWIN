from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
import struct
from typing import Optional, Tuple


SYNC = b"\xAA\x55"
VERSION = 1
HEADER_SIZE = 10
CRC_SIZE = 2
MAX_FRAME_SIZE = 64


class PacketType(IntEnum):
    TELEMETRY = 0x01
    TWIN_STATE = 0x02


class ProtocolError(Exception):
    """Base class for protocol errors."""


class PacketValidationError(ProtocolError):
    """Raised when a frame fails header or CRC validation."""


class IncompleteFrameError(ProtocolError):
    """Raised when more bytes are needed to complete a frame."""


def crc16_ccitt_false(data: bytes, initial: int = 0xFFFF) -> int:
    crc = initial & 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


@dataclass(frozen=True)
class TelemetryPacket:
    sequence: int
    timestamp_us: int
    duty_a: float
    duty_b: float
    duty_c: float
    vdc: float


@dataclass(frozen=True)
class TwinStatePacket:
    sequence: int
    timestamp_us: int
    ia: float
    ib: float
    ic: float
    rotor_speed: float
    rotor_angle: float
    electrical_angle: float
    torque: float
    temperature: float
    hall: int
    encoder: int
    fault_flags: int


def _pack_header(packet_type: PacketType, sequence: int, payload_len: int) -> bytes:
    return struct.pack("<2sBBHHH", SYNC, VERSION, int(packet_type), sequence & 0xFFFF, payload_len, 0)


def _unpack_header(frame: bytes) -> Tuple[int, int, int, int, int]:
    sync, version, packet_type, sequence, payload_len, reserved = struct.unpack("<2sBBHHH", frame[:HEADER_SIZE])
    if sync != SYNC:
        raise PacketValidationError("invalid sync bytes")
    if version != VERSION:
        raise PacketValidationError(f"unsupported protocol version: {version}")
    if reserved != 0:
        raise PacketValidationError("reserved header field must be zero")
    return packet_type, sequence, payload_len, version, reserved


def encode_telemetry(packet: TelemetryPacket) -> bytes:
    payload = struct.pack(
        "<Qffff",
        packet.timestamp_us,
        packet.duty_a,
        packet.duty_b,
        packet.duty_c,
        packet.vdc,
    )
    header = _pack_header(PacketType.TELEMETRY, packet.sequence, len(payload))
    crc = crc16_ccitt_false(header[2:] + payload)
    return header + payload + struct.pack("<H", crc)


def encode_twin_state(packet: TwinStatePacket) -> bytes:
    payload = struct.pack(
        "<QffffffffBHI",
        packet.timestamp_us,
        packet.ia,
        packet.ib,
        packet.ic,
        packet.rotor_speed,
        packet.rotor_angle,
        packet.electrical_angle,
        packet.torque,
        packet.temperature,
        packet.hall,
        packet.encoder,
        packet.fault_flags,
    )
    header = _pack_header(PacketType.TWIN_STATE, packet.sequence, len(payload))
    crc = crc16_ccitt_false(header[2:] + payload)
    return header + payload + struct.pack("<H", crc)


def _validate_and_split(frame: bytes) -> Tuple[PacketType, int, bytes]:
    if len(frame) < HEADER_SIZE + CRC_SIZE:
        raise IncompleteFrameError("frame is too short")
    if frame[:2] != SYNC:
        raise PacketValidationError("invalid sync bytes")

    packet_type_raw, sequence, payload_len, _, _ = _unpack_header(frame)
    try:
        packet_type = PacketType(packet_type_raw)
    except ValueError as exc:
        raise PacketValidationError(f"unknown packet type: {packet_type_raw}") from exc

    expected_len = HEADER_SIZE + payload_len + CRC_SIZE
    if len(frame) != expected_len:
        raise PacketValidationError(
            f"length mismatch: expected {expected_len} bytes, got {len(frame)} bytes"
        )

    payload = frame[HEADER_SIZE:HEADER_SIZE + payload_len]
    received_crc = struct.unpack("<H", frame[-CRC_SIZE:])[0]
    calculated_crc = crc16_ccitt_false(frame[2:-CRC_SIZE])
    if received_crc != calculated_crc:
        raise PacketValidationError("CRC mismatch")

    if packet_type == PacketType.TELEMETRY and payload_len != 24:
        raise PacketValidationError("invalid telemetry payload length")
    if packet_type == PacketType.TWIN_STATE and payload_len != 47:
        raise PacketValidationError("invalid twin state payload length")

    return packet_type, sequence, payload


def decode_frame(frame: bytes) -> TelemetryPacket | TwinStatePacket:
    packet_type, sequence, payload = _validate_and_split(frame)

    if packet_type == PacketType.TELEMETRY:
        timestamp_us, duty_a, duty_b, duty_c, vdc = struct.unpack("<Qffff", payload)
        return TelemetryPacket(
            sequence=sequence,
            timestamp_us=timestamp_us,
            duty_a=duty_a,
            duty_b=duty_b,
            duty_c=duty_c,
            vdc=vdc,
        )

    (
        timestamp_us,
        ia,
        ib,
        ic,
        rotor_speed,
        rotor_angle,
        electrical_angle,
        torque,
        temperature,
        hall,
        encoder,
        fault_flags,
    ) = struct.unpack("<QffffffffBHI", payload)
    return TwinStatePacket(
        sequence=sequence,
        timestamp_us=timestamp_us,
        ia=ia,
        ib=ib,
        ic=ic,
        rotor_speed=rotor_speed,
        rotor_angle=rotor_angle,
        electrical_angle=electrical_angle,
        torque=torque,
        temperature=temperature,
        hall=hall,
        encoder=encoder,
        fault_flags=fault_flags,
    )


class FrameBuffer:
    """Incremental stream parser for a byte-oriented serial link."""

    def __init__(self) -> None:
        self._buffer = bytearray()

    def feed(self, data: bytes) -> list[TelemetryPacket | TwinStatePacket]:
        self._buffer.extend(data)
        packets: list[TelemetryPacket | TwinStatePacket] = []

        while True:
            frame = self._extract_next_frame()
            if frame is None:
                break
            try:
                packets.append(decode_frame(frame))
            except ProtocolError:
                continue

        return packets

    def _extract_next_frame(self) -> Optional[bytes]:
        while len(self._buffer) >= 2 and self._buffer[:2] != SYNC:
            del self._buffer[0]

        if len(self._buffer) < HEADER_SIZE:
            return None

        try:
            _, _, payload_len, _, _ = _unpack_header(bytes(self._buffer[:HEADER_SIZE]))
        except PacketValidationError:
            del self._buffer[0]
            return None

        if payload_len > MAX_FRAME_SIZE:
            del self._buffer[0]
            return None

        frame_len = HEADER_SIZE + payload_len + CRC_SIZE
        if len(self._buffer) < frame_len:
            return None

        frame = bytes(self._buffer[:frame_len])
        del self._buffer[:frame_len]
        return frame
