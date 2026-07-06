from __future__ import annotations

from dataclasses import dataclass
from math import isfinite, tau
from typing import Protocol

from backend.app.models.motor_parameters import MotorParameters


class MechanicalModelError(ValueError):
    """Raised when mechanical model inputs or parameters are invalid."""


@dataclass(frozen=True)
class MechanicalInputs:
    Torque: float
    LoadTorque: float | None = None

    def validate(self) -> None:
        if not isinstance(self.Torque, (int, float)) or not isfinite(float(self.Torque)):
            raise MechanicalModelError("Torque must be a finite number")
        if self.LoadTorque is not None and (
            not isinstance(self.LoadTorque, (int, float)) or not isfinite(float(self.LoadTorque))
        ):
            raise MechanicalModelError("LoadTorque must be a finite number")


@dataclass(frozen=True)
class MechanicalState:
    Speed: float = 0.0
    MechanicalAngle: float = 0.0

    def validate(self) -> None:
        for name, value in (("Speed", self.Speed), ("MechanicalAngle", self.MechanicalAngle)):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise MechanicalModelError(f"{name} must be a finite number")


@dataclass(frozen=True)
class MechanicalOutputs:
    Speed: float
    MechanicalAngle: float
    ElectricalAngle: float


class TorqueLoadModel(Protocol):
    """
    Strategy interface for disturbance load torque.

    Implementations can provide time-varying load profiles, friction terms,
    or external mechanical coupling.
    """

    def load_torque(self, speed: float, mechanical_angle: float, parameters: MotorParameters) -> float:
        ...


class ConstantLoadTorqueModel:
    """Default load torque strategy using the value stored in motor parameters."""

    def load_torque(self, speed: float, mechanical_angle: float, parameters: MotorParameters) -> float:
        return parameters.LoadTorque


class EulerIntegrator:
    """Explicit Euler integration: x[k+1] = x[k] + dt * dx/dt."""

    @staticmethod
    def step(value: float, derivative: float, dt: float) -> float:
        if not isinstance(dt, (int, float)) or not isfinite(float(dt)) or float(dt) <= 0.0:
            raise MechanicalModelError("dt must be a finite number greater than 0")
        return float(value) + float(dt) * float(derivative)


class MechanicalModel:
    """
    Rotor mechanical dynamics solver.

    The model solves:

        dω/dt = (T_e - T_L - B * ω) / J
        dθ_m/dt = ω

    Where:
    - ω is rotor speed in rad/s
    - θ_m is mechanical angle in rad
    - T_e is electromagnetic torque
    - T_L is load torque
    - B is viscous friction coefficient
    - J is rotor inertia

    Electrical angle is derived as:

        θ_e = p * θ_m

    where p is the pole-pair count.
    """

    def __init__(
        self,
        parameters: MotorParameters,
        torque_load_model: TorqueLoadModel | None = None,
        integrator: EulerIntegrator | None = None,
        wrap_mechanical_angle: bool = False,
    ) -> None:
        parameters.validate()
        self.parameters = parameters
        self.torque_load_model = torque_load_model or ConstantLoadTorqueModel()
        self.integrator = integrator or EulerIntegrator()
        self.wrap_mechanical_angle = wrap_mechanical_angle
        self.state = MechanicalState()

    def reset(self, state: MechanicalState | None = None) -> None:
        if state is None:
            self.state = MechanicalState()
            return
        state.validate()
        self.state = state

    def step(self, inputs: MechanicalInputs, dt: float) -> MechanicalOutputs:
        inputs.validate()
        self.state.validate()

        inertia = self.parameters.RotorInertia
        if inertia <= 0.0:
            raise MechanicalModelError("RotorInertia must be greater than 0")

        load_torque = (
            float(inputs.LoadTorque)
            if inputs.LoadTorque is not None
            else float(self.torque_load_model.load_torque(self.state.Speed, self.state.MechanicalAngle, self.parameters))
        )
        if not isfinite(load_torque):
            raise MechanicalModelError("load torque must be finite")

        speed_derivative = (
            float(inputs.Torque)
            - load_torque
            - self.parameters.Friction * self.state.Speed
        ) / inertia
        angle_derivative = self.state.Speed

        speed = self.integrator.step(self.state.Speed, speed_derivative, dt)
        mechanical_angle = self.integrator.step(self.state.MechanicalAngle, angle_derivative, dt)
        if self.wrap_mechanical_angle:
            mechanical_angle = mechanical_angle % tau

        electrical_angle = self._electrical_angle(mechanical_angle)

        self.state = MechanicalState(Speed=speed, MechanicalAngle=mechanical_angle)
        return MechanicalOutputs(
            Speed=speed,
            MechanicalAngle=mechanical_angle,
            ElectricalAngle=electrical_angle,
        )

    def _electrical_angle(self, mechanical_angle: float) -> float:
        return self.parameters.PolePairs * mechanical_angle

    def validate(self) -> None:
        self.parameters.validate()
        self.state.validate()

