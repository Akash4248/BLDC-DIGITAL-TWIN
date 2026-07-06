from __future__ import annotations

from dataclasses import dataclass

from backend.app.serial.protocol import TelemetryPacket, TwinStatePacket, decode_frame, encode_twin_state
from backend.app.twin.engine import TwinSimulationEngine


class TwinLoopError(ValueError):
    """Raised when loop processing fails."""


@dataclass(frozen=True)
class TwinLoopResult:
    feedback_frame: bytes
    feedback_packet: TwinStatePacket


class TwinUsbLoop:
    """
    Raw USB frame processor for the digital twin.

    This is the transport-facing wrapper around the deterministic simulation
    engine. It accepts a binary telemetry frame, steps the engine once, and
    returns a binary feedback frame.
    """

    def __init__(self, engine: TwinSimulationEngine) -> None:
        self.engine = engine

    def process(self, frame: bytes) -> TwinLoopResult:
        packet = decode_frame(frame)
        if not isinstance(packet, TelemetryPacket):
            raise TwinLoopError("expected telemetry packet from USB input")

        result = self.engine.step(packet)
        feedback_frame = encode_twin_state(result.feedback)
        return TwinLoopResult(feedback_frame=feedback_frame, feedback_packet=result.feedback)

