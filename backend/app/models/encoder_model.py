from __future__ import annotations

from dataclasses import dataclass
from math import fmod, isfinite, pi


class EncoderModelError(ValueError):
    """Raised when encoder inputs or configuration are invalid."""


@dataclass(frozen=True)
class EncoderConfig:
    """
    Incremental encoder configuration.

    PPR is the number of pulses per revolution on channel A. With x4 decoding,
    one mechanical revolution contains 4 * PPR edges.
    """

    PPR: int
    ElectricalOffset: float = 0.0
    IndexWidthRad: float = 0.0
    TransitionTime: float = 0.0

    def validate(self) -> None:
        if not isinstance(self.PPR, int):
            raise EncoderModelError("PPR must be an integer")
        if self.PPR <= 0:
            raise EncoderModelError("PPR must be greater than 0")
        for name, value in (
            ("ElectricalOffset", self.ElectricalOffset),
            ("IndexWidthRad", self.IndexWidthRad),
            ("TransitionTime", self.TransitionTime),
        ):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise EncoderModelError(f"{name} must be a finite number")
        if float(self.IndexWidthRad) < 0.0:
            raise EncoderModelError("IndexWidthRad must be greater than or equal to 0")
        if float(self.TransitionTime) < 0.0:
            raise EncoderModelError("TransitionTime must be greater than or equal to 0")


@dataclass(frozen=True)
class EncoderInputs:
    MechanicalAngle: float

    def validate(self) -> None:
        if not isinstance(self.MechanicalAngle, (int, float)) or not isfinite(float(self.MechanicalAngle)):
            raise EncoderModelError("MechanicalAngle must be a finite number")


@dataclass(frozen=True)
class EncoderOutputs:
    EncoderA: float
    EncoderB: float
    IndexPulse: float


@dataclass
class EncoderState:
    EncoderA: float = 0.0
    EncoderB: float = 0.0
    IndexPulse: float = 0.0

    def validate(self) -> None:
        for name, value in (("EncoderA", self.EncoderA), ("EncoderB", self.EncoderB), ("IndexPulse", self.IndexPulse)):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise EncoderModelError(f"{name} must be a finite number")


class EulerIntegrator:
    """Explicit Euler helper for signal transitions."""

    @staticmethod
    def step(value: float, derivative: float, dt: float) -> float:
        if not isinstance(dt, (int, float)) or not isfinite(float(dt)) or float(dt) <= 0.0:
            raise EncoderModelError("dt must be a finite number greater than 0")
        return float(value) + float(dt) * float(derivative)


class EncoderModel:
    """
    Incremental encoder simulator.

    Quadrature A/B are generated from the mechanical angle and configurable PPR.
    Index pulse is asserted once per revolution in a narrow electrical window.

    A/B phase relationship:

        A = square(sin(2*pi * PPR * theta / 2*pi))
        B = square(sin(2*pi * PPR * theta / 2*pi + pi/2))

    Simplified:

        A = 1 if sin(PPR * theta + offset) >= 0 else 0
        B = 1 if sin(PPR * theta + offset + pi/2) >= 0 else 0
    """

    def __init__(
        self,
        config: EncoderConfig,
        integrator: EulerIntegrator | None = None,
    ) -> None:
        self.config = config
        self.config.validate()
        self.integrator = integrator or EulerIntegrator()
        self.state = EncoderState()

    def reset(self, state: EncoderState | None = None) -> None:
        if state is None:
            self.state = EncoderState()
            return
        state.validate()
        self.state = state

    def step(self, inputs: EncoderInputs, dt: float) -> EncoderOutputs:
        inputs.validate()
        self.config.validate()
        self.state.validate()

        target_a = self._quadrature_a(inputs.MechanicalAngle)
        target_b = self._quadrature_b(inputs.MechanicalAngle)
        target_index = self._index_pulse(inputs.MechanicalAngle)

        if self.config.TransitionTime <= 0.0:
            self.state = EncoderState(float(target_a), float(target_b), float(target_index))
            return EncoderOutputs(self.state.EncoderA, self.state.EncoderB, self.state.IndexPulse)

        alpha = 1.0 / self.config.TransitionTime
        encoder_a = self.integrator.step(self.state.EncoderA, alpha * (target_a - self.state.EncoderA), dt)
        encoder_b = self.integrator.step(self.state.EncoderB, alpha * (target_b - self.state.EncoderB), dt)
        index_pulse = self.integrator.step(self.state.IndexPulse, alpha * (target_index - self.state.IndexPulse), dt)

        self.state = EncoderState(
            EncoderA=self._clamp01(encoder_a),
            EncoderB=self._clamp01(encoder_b),
            IndexPulse=self._clamp01(index_pulse),
        )
        return EncoderOutputs(self.state.EncoderA, self.state.EncoderB, self.state.IndexPulse)

    def _quadrature_a(self, mechanical_angle: float) -> int:
        angle = self._wrapped_angle(mechanical_angle)
        return 1 if self._encoder_phase(angle) >= 0.0 else 0

    def _quadrature_b(self, mechanical_angle: float) -> int:
        angle = self._wrapped_angle(mechanical_angle)
        return 1 if self._encoder_phase(angle + pi / 2.0) >= 0.0 else 0

    def _index_pulse(self, mechanical_angle: float) -> int:
        angle = self._wrapped_angle(mechanical_angle)
        return 1 if angle <= self.config.IndexWidthRad else 0

    def _encoder_phase(self, mechanical_angle: float) -> float:
        return self.config.PPR * mechanical_angle + self.config.ElectricalOffset

    @staticmethod
    def _wrapped_angle(angle: float) -> float:
        wrapped = fmod(angle, 2.0 * pi)
        if wrapped < 0.0:
            wrapped += 2.0 * pi
        return wrapped

    @staticmethod
    def _clamp01(value: float) -> float:
        return max(0.0, min(1.0, float(value)))

