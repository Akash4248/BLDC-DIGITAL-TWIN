from __future__ import annotations

from enum import Enum
from typing import Any

from pydantic import BaseModel, Field


class SimulationControlAction(str, Enum):
    start = "start"
    stop = "stop"
    reset = "reset"


class MotorParametersUpdate(BaseModel):
    Rs: float
    Ld: float
    Lq: float
    PolePairs: int
    FluxLinkage: float
    Kt: float
    Ke: float
    RotorInertia: float
    Friction: float
    LoadTorque: float
    Vdc: float


class FaultType(str, Enum):
    open_phase = "open_phase"
    hall_failure = "hall_failure"
    encoder_failure = "encoder_failure"
    current_sensor_offset = "current_sensor_offset"
    locked_rotor = "locked_rotor"
    high_load = "high_load"
    low_voltage = "low_voltage"
    short_circuit = "short_circuit"


class FaultInjectionRequest(BaseModel):
    fault_type: FaultType
    enabled: bool = True
    value: float | None = None


class TelemetryStepRequest(BaseModel):
    sequence: int = Field(ge=0, le=65535)
    timestamp_us: int = Field(ge=0)
    duty_a: float
    duty_b: float
    duty_c: float
    vdc: float


class ControlResponse(BaseModel):
    status: str
    running: bool


class SimulationStateResponse(BaseModel):
    running: bool
    sample_rate_hz: int
    step_index: int
    parameters: dict[str, Any]
    faults: list[dict[str, Any]]


class LogEntryResponse(BaseModel):
    timestamp: str
    level: str
    event: str
    detail: dict[str, Any]


class FaultStatusResponse(BaseModel):
    status: str
    faults: list[dict[str, Any]]
