from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from math import isfinite
from typing import Protocol


class InverterModelError(ValueError):
    """Raised when inverter inputs or configuration are invalid."""


class ModulationMethod(str, Enum):
    SINUSOIDAL_PWM = "sinusoidal_pwm"
    SVPWM = "svpwm"


@dataclass(frozen=True)
class InverterInputs:
    DutyA: float
    DutyB: float
    DutyC: float
    Vdc: float

    def validate(self) -> None:
        for name, value in (
            ("DutyA", self.DutyA),
            ("DutyB", self.DutyB),
            ("DutyC", self.DutyC),
            ("Vdc", self.Vdc),
        ):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise InverterModelError(f"{name} must be a finite number")

        for name, value in (("DutyA", self.DutyA), ("DutyB", self.DutyB), ("DutyC", self.DutyC)):
            if not 0.0 <= float(value) <= 1.0:
                raise InverterModelError(f"{name} must be in the range [0, 1]")

        if float(self.Vdc) <= 0.0:
            raise InverterModelError("Vdc must be greater than 0")


@dataclass(frozen=True)
class InverterOutputs:
    Va: float
    Vb: float
    Vc: float


class DeadTimeCompensationStrategy(Protocol):
    """
    Interface for duty correction before voltage synthesis.

    Implementations can adjust duty commands using device-specific timing,
    current direction, and gate-driver characteristics.
    """

    def compensate(
        self,
        duty_a: float,
        duty_b: float,
        duty_c: float,
        vdc: float,
    ) -> tuple[float, float, float]:
        ...


class InverterModel:
    """
    Deterministic inverter voltage synthesis.

    The model maps normalized duty cycles to phase voltages using either
    sinusoidal PWM or SVPWM-style common-mode removal.
    """

    def __init__(
        self,
        modulation: ModulationMethod = ModulationMethod.SVPWM,
        dead_time_compensator: DeadTimeCompensationStrategy | None = None,
    ) -> None:
        self.modulation = modulation
        self.dead_time_compensator = dead_time_compensator

    def set_dead_time_compensator(
        self, compensator: DeadTimeCompensationStrategy | None
    ) -> None:
        self.dead_time_compensator = compensator

    def compute(self, inputs: InverterInputs) -> InverterOutputs:
        inputs.validate()

        duty_a, duty_b, duty_c = inputs.DutyA, inputs.DutyB, inputs.DutyC
        if self.dead_time_compensator is not None:
            duty_a, duty_b, duty_c = self.dead_time_compensator.compensate(
                duty_a, duty_b, duty_c, inputs.Vdc
            )
            self._validate_duty("DutyA", duty_a)
            self._validate_duty("DutyB", duty_b)
            self._validate_duty("DutyC", duty_c)

        if self.modulation == ModulationMethod.SINUSOIDAL_PWM:
            return self._compute_sinusoidal_pwm(duty_a, duty_b, duty_c, inputs.Vdc)
        if self.modulation == ModulationMethod.SVPWM:
            return self._compute_svpwm(duty_a, duty_b, duty_c, inputs.Vdc)

        raise InverterModelError(f"unsupported modulation method: {self.modulation}")

    @staticmethod
    def _validate_duty(name: str, duty: float) -> None:
        if not isinstance(duty, (int, float)) or not isfinite(float(duty)):
            raise InverterModelError(f"{name} must be a finite number")
        if not 0.0 <= float(duty) <= 1.0:
            raise InverterModelError(f"{name} must be in the range [0, 1]")

    @staticmethod
    def _compute_sinusoidal_pwm(duty_a: float, duty_b: float, duty_c: float, vdc: float) -> InverterOutputs:
        # Phase voltage relative to the inverter midpoint.
        half_bus = 0.5 * vdc
        return InverterOutputs(
            Va=(2.0 * duty_a - 1.0) * half_bus,
            Vb=(2.0 * duty_b - 1.0) * half_bus,
            Vc=(2.0 * duty_c - 1.0) * half_bus,
        )

    @staticmethod
    def _compute_svpwm(duty_a: float, duty_b: float, duty_c: float, vdc: float) -> InverterOutputs:
        # Remove the common-mode component so the phase voltages sum to zero.
        duty_avg = (duty_a + duty_b + duty_c) / 3.0
        return InverterOutputs(
            Va=(duty_a - duty_avg) * vdc,
            Vb=(duty_b - duty_avg) * vdc,
            Vc=(duty_c - duty_avg) * vdc,
        )

