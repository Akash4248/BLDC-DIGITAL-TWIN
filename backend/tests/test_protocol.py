from backend.app.serial.protocol import (
    FrameBuffer,
    TelemetryPacket,
    TwinStatePacket,
    decode_frame,
    encode_telemetry,
    encode_twin_state,
)

import pytest


def test_encode_decode_telemetry_roundtrip() -> None:
    original = TelemetryPacket(
        sequence=42,
        timestamp_us=123456789,
        duty_a=0.1,
        duty_b=0.2,
        duty_c=0.3,
        vdc=24.0,
    )

    frame = encode_telemetry(original)
    decoded = decode_frame(frame)

    assert isinstance(decoded, TelemetryPacket)
    assert decoded.sequence == original.sequence
    assert decoded.timestamp_us == original.timestamp_us
    assert decoded.duty_a == pytest.approx(original.duty_a)
    assert decoded.duty_b == pytest.approx(original.duty_b)
    assert decoded.duty_c == pytest.approx(original.duty_c)
    assert decoded.vdc == pytest.approx(original.vdc)


def test_encode_decode_twin_state_roundtrip() -> None:
    original = TwinStatePacket(
        sequence=7,
        ia=1.0,
        ib=-0.5,
        ic=-0.5,
        rotor_angle=1.57,
        rotor_speed=120.0,
    )

    frame = encode_twin_state(original)
    decoded = decode_frame(frame)

    assert isinstance(decoded, TwinStatePacket)
    assert decoded.sequence == original.sequence
    assert decoded.ia == pytest.approx(original.ia)
    assert decoded.ib == pytest.approx(original.ib)
    assert decoded.ic == pytest.approx(original.ic)
    assert decoded.rotor_angle == pytest.approx(original.rotor_angle)
    assert decoded.rotor_speed == pytest.approx(original.rotor_speed)


def test_stream_parser_resynchronizes() -> None:
    packet = TelemetryPacket(
        sequence=1,
        timestamp_us=99,
        duty_a=0.4,
        duty_b=0.5,
        duty_c=0.6,
        vdc=48.0,
    )
    frame = encode_telemetry(packet)

    parser = FrameBuffer()
    packets = parser.feed(b"\x00\x11garbage" + frame[:8])
    assert packets == []

    packets = parser.feed(frame[8:])
    assert len(packets) == 1
    assert isinstance(packets[0], TelemetryPacket)
