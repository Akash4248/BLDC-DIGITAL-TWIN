from __future__ import annotations

from dataclasses import dataclass, asdict, replace
from enum import Enum
from typing import Any

from backend.app.serial.protocol import TelemetryPacket, TwinStatePacket
from backend.app.twin.engine import TwinStepResult


class FaultInjectionError(ValueError):
    """Raised when a fault is invalid or cannot be applied."""


class FaultType(str, Enum):
    open_phase = "open_phase"
    hall_failure = "hall_failure"
    encoder_failure = "encoder_failure"
    current_sensor_offset = "current_sensor_offset"
    locked_rotor = "locked_rotor"
    high_load = "high_load"
    low_voltage = "low_voltage"
    short_circuit = "short_circuit"


@dataclass
class Fault:
    fault_type: FaultType
    enabled: bool = True
    value: float | None = None

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


class FaultManager:
    def __init__(self) -> None:
        self._faults: dict[FaultType, Fault] = {}
        self._frozen_hall = None
        self._frozen_encoder = None
        self._frozen_mechanical = None

    def has(self, fault_type: FaultType) -> bool:
        fault = self._faults.get(fault_type)
        return bool(fault and fault.enabled)

    def upsert(self, fault_type: FaultType, enabled: bool = True, value: float | None = None) -> Fault:
        fault = Fault(fault_type=fault_type, enabled=enabled, value=value)
        self._faults[fault_type] = fault
        return fault

    def clear(self) -> None:
        self._faults.clear()
        self._frozen_hall = None
        self._frozen_encoder = None
        self._frozen_mechanical = None

    def list(self) -> list[dict[str, Any]]:
        return [fault.to_dict() for fault in self._faults.values()]

    def apply_telemetry(self, telemetry: TelemetryPacket) -> TelemetryPacket:
        if self.has(FaultType.short_circuit):
            telemetry = replace(telemetry, duty_a=0.5, duty_b=0.5, duty_c=0.5)

        if self.has(FaultType.open_phase):
            phase = 0
            fault = self._faults[FaultType.open_phase]
            if fault.value is not None:
                phase = int(fault.value) % 3
            duties = [telemetry.duty_a, telemetry.duty_b, telemetry.duty_c]
            duties[phase] = 0.0
            telemetry = replace(telemetry, duty_a=duties[0], duty_b=duties[1], duty_c=duties[2])

        if self.has(FaultType.low_voltage):
            fault = self._faults[FaultType.low_voltage]
            scale = float(fault.value if fault.value is not None else 0.6)
            telemetry = replace(telemetry, vdc=max(0.0, telemetry.vdc * scale))

        return telemetry

    def load_torque_delta(self) -> float:
        if not self.has(FaultType.high_load):
            return 0.0
        fault = self._faults[FaultType.high_load]
        return float(fault.value if fault.value is not None else 2.0)

    def apply(self, result: TwinStepResult) -> TwinStepResult:
        current_sensor_outputs = result.current_sensors
        hall_outputs = result.hall
        encoder_outputs = result.encoder
        feedback = result.feedback
        mechanical_outputs = result.mechanical

        offset_fault = self._faults.get(FaultType.current_sensor_offset)
        if offset_fault and offset_fault.enabled:
            offset = int(offset_fault.value if offset_fault.value is not None else 40.0)
            current_sensor_outputs = type(current_sensor_outputs)(
                ADCA=current_sensor_outputs.ADCA + offset,
                ADCB=current_sensor_outputs.ADCB + offset,
                ADCC=current_sensor_outputs.ADCC + offset,
            )

        if (freeze := self._faults.get(FaultType.hall_failure)) and freeze.enabled:
            if self._frozen_hall is None:
                self._frozen_hall = result.hall
            hall_outputs = self._frozen_hall

        if (freeze := self._faults.get(FaultType.encoder_failure)) and freeze.enabled:
            if self._frozen_encoder is None:
                self._frozen_encoder = result.encoder
            encoder_outputs = self._frozen_encoder

        if (locked := self._faults.get(FaultType.locked_rotor)) and locked.enabled:
            if self._frozen_mechanical is None:
                self._frozen_mechanical = result.mechanical
            mechanical_outputs = self._frozen_mechanical
            feedback = TwinStatePacket(
                sequence=feedback.sequence,
                ia=feedback.ia,
                ib=feedback.ib,
                ic=feedback.ic,
                rotor_angle=mechanical_outputs.ElectricalAngle,
                rotor_speed=0.0,
            )

        return type(result)(
            telemetry=result.telemetry,
            inverter=result.inverter,
            electrical=result.electrical,
            mechanical=mechanical_outputs,
            hall=hall_outputs,
            encoder=encoder_outputs,
            current_sensors=current_sensor_outputs,
            feedback=feedback,
            electromagnetic_torque=result.electromagnetic_torque,
        )
