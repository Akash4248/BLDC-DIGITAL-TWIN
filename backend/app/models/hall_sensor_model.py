from __future__ import annotations

from dataclasses import dataclass
from math import fmod, isfinite, pi
from typing import Iterable


class HallSensorModelError(ValueError):
    """Raised when hall sensor inputs or configuration are invalid."""


@dataclass(frozen=True)
class HallSensorConfig:
    """
    Configurable Hall sensor placement in electrical radians.

    Offsets shift the electrical angle seen by each sensor so the model can be
    aligned to a real motor / PCB installation.
    """

    ElectricalOffset: float = 0.0
    TransitionTime: float = 0.0

    def validate(self) -> None:
        for name, value in (
            ("ElectricalOffset", self.ElectricalOffset),
            ("TransitionTime", self.TransitionTime),
        ):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise HallSensorModelError(f"{name} must be a finite number")
        if float(self.TransitionTime) < 0.0:
            raise HallSensorModelError("TransitionTime must be greater than or equal to 0")


@dataclass(frozen=True)
class HallSensorInputs:
    ElectricalAngle: float

    def validate(self) -> None:
        if not isinstance(self.ElectricalAngle, (int, float)) or not isfinite(float(self.ElectricalAngle)):
            raise HallSensorModelError("ElectricalAngle must be a finite number")


@dataclass(frozen=True)
class HallSensorOutputs:
    HallA: float
    HallB: float
    HallC: float


@dataclass
class HallSensorState:
    HallA: float = 0.0
    HallB: float = 0.0
    HallC: float = 0.0

    def validate(self) -> None:
        for name, value in (("HallA", self.HallA), ("HallB", self.HallB), ("HallC", self.HallC)):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise HallSensorModelError(f"{name} must be a finite number")


class EulerIntegrator:
    """Explicit Euler helper for smooth transitions."""

    @staticmethod
    def step(value: float, derivative: float, dt: float) -> float:
        if not isinstance(dt, (int, float)) or not isfinite(float(dt)) or float(dt) <= 0.0:
            raise HallSensorModelError("dt must be a finite number greater than 0")
        return float(value) + float(dt) * float(derivative)


class HallSensorModel:
    """
    Digital Hall sensor generator.

    Each Hall channel is a 3-state sector detector with six electrical sectors.

    Sector angle:
        sector = floor(mod(theta + offset, 2*pi) / (pi/3))

    The canonical 3-bit Hall pattern is generated as:

        sector 0 -> 101
        sector 1 -> 100
        sector 2 -> 110
        sector 3 -> 010
        sector 4 -> 011
        sector 5 -> 001

    Transition animation is optional. When enabled, the model eases each
    channel toward its target value over TransitionTime using Euler integration.
    """

    _SECTOR_TO_PATTERN: tuple[tuple[int, int, int], ...] = (
        (1, 0, 1),
        (1, 0, 0),
        (1, 1, 0),
        (0, 1, 0),
        (0, 1, 1),
        (0, 0, 1),
    )

    def __init__(
        self,
        config: HallSensorConfig | None = None,
        integrator: EulerIntegrator | None = None,
    ) -> None:
        self.config = config or HallSensorConfig()
        self.config.validate()
        self.integrator = integrator or EulerIntegrator()
        self.state = HallSensorState(*self._sector_pattern(0.0, self.config))

    def reset(self, state: HallSensorState | None = None) -> None:
        if state is None:
            self.state = HallSensorState(*self._sector_pattern(0.0, self.config))
            return
        state.validate()
        self.state = state

    def step(self, inputs: HallSensorInputs, dt: float) -> HallSensorOutputs:
        inputs.validate()
        self.config.validate()
        self.state.validate()

        target_a, target_b, target_c = self._sector_pattern(inputs.ElectricalAngle, self.config)

        if self.config.TransitionTime <= 0.0:
            self.state = HallSensorState(float(target_a), float(target_b), float(target_c))
            return HallSensorOutputs(HallA=self.state.HallA, HallB=self.state.HallB, HallC=self.state.HallC)

        alpha = 1.0 / self.config.TransitionTime
        hall_a = self.integrator.step(self.state.HallA, alpha * (target_a - self.state.HallA), dt)
        hall_b = self.integrator.step(self.state.HallB, alpha * (target_b - self.state.HallB), dt)
        hall_c = self.integrator.step(self.state.HallC, alpha * (target_c - self.state.HallC), dt)

        self.state = HallSensorState(
            HallA=self._clamp01(hall_a),
            HallB=self._clamp01(hall_b),
            HallC=self._clamp01(hall_c),
        )
        return HallSensorOutputs(HallA=self.state.HallA, HallB=self.state.HallB, HallC=self.state.HallC)

    @classmethod
    def _sector_pattern(cls, electrical_angle: float, config: HallSensorConfig) -> tuple[int, int, int]:
        sector = cls._sector_index(electrical_angle + config.ElectricalOffset)
        return cls._SECTOR_TO_PATTERN[sector]

    @staticmethod
    def _sector_index(angle: float) -> int:
        wrapped = fmod(angle, 2.0 * pi)
        if wrapped < 0.0:
            wrapped += 2.0 * pi
        return int(wrapped // (pi / 3.0)) % 6

    @staticmethod
    def _clamp01(value: float) -> float:
        return max(0.0, min(1.0, float(value)))
