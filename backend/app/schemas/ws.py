from __future__ import annotations

from pydantic import BaseModel


class TelemetrySnapshot(BaseModel):
    telemetry: dict
    current: dict
    speed: float
    torque: float
    rotor_angle: float
    hall: dict
    encoder: dict

