from __future__ import annotations

from dataclasses import dataclass
from math import isfinite, pi, sin
from typing import Protocol

from backend.app.models.motor_parameters import MotorParameterError, MotorParameters


class ElectricalModelError(ValueError):
    """Raised when the electrical model inputs or state are invalid."""


@dataclass(frozen=True)
class ElectricalInputs:
    Va: float
    Vb: float
    Vc: float
    RotorAngle: float
    Speed: float

    def validate(self) -> None:
        for name, value in (
            ("Va", self.Va),
            ("Vb", self.Vb),
            ("Vc", self.Vc),
            ("RotorAngle", self.RotorAngle),
            ("Speed", self.Speed),
        ):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise ElectricalModelError(f"{name} must be a finite number")


@dataclass(frozen=True)
class ElectricalState:
    Ia: float = 0.0
    Ib: float = 0.0
    Ic: float = 0.0

    def validate(self) -> None:
        for name, value in (("Ia", self.Ia), ("Ib", self.Ib), ("Ic", self.Ic)):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise ElectricalModelError(f"{name} must be a finite number")


@dataclass(frozen=True)
class ElectricalOutputs:
    Ia: float
    Ib: float
    Ic: float


class BackEMFModel(Protocol):
    """
    Strategy interface for phase back-EMF synthesis.

    The rotor-angle convention is mechanical angle in radians unless an
    implementation documents otherwise.
    """

    def phase_emf(self, rotor_angle: float, speed: float, parameters: MotorParameters) -> tuple[float, float, float]:
        ...


class SineBackEMFModel:
    """
    Sinusoidal phase back-EMF model.

    e_a = K_e * omega_m * sin(theta_e)
    e_b = K_e * omega_m * sin(theta_e - 2*pi/3)
    e_c = K_e * omega_m * sin(theta_e + 2*pi/3)

    where:
    - theta_e = pole_pairs * theta_m
    - omega_m is mechanical speed
    - K_e is the back-EMF constant
    """

    def phase_emf(self, rotor_angle: float, speed: float, parameters: MotorParameters) -> tuple[float, float, float]:
        theta_e = parameters.PolePairs * rotor_angle
        amplitude = parameters.Ke * speed
        return (
            amplitude * sin(theta_e),
            amplitude * sin(theta_e - 2.0 * pi / 3.0),
            amplitude * sin(theta_e + 2.0 * pi / 3.0),
        )


class EulerIntegrator:
    """
    First-order explicit Euler integrator.

    x[k+1] = x[k] + dt * dx/dt
    """

    @staticmethod
    def step(value: float, derivative: float, dt: float) -> float:
        if not isinstance(dt, (int, float)) or not isfinite(float(dt)) or float(dt) <= 0.0:
            raise ElectricalModelError("dt must be a finite number greater than 0")
        return float(value) + float(dt) * float(derivative)


class ElectricalModel:
    """
    Phase-domain electrical solver.

    Per-phase current dynamics:

        dIa/dt = (Va - Rs * Ia - e_a) / Ls
        dIb/dt = (Vb - Rs * Ib - e_b) / Ls
        dIc/dt = (Vc - Rs * Ic - e_c) / Ls

    where:
    - Va, Vb, Vc are phase voltages
    - e_a, e_b, e_c are back-EMF voltages
    - Rs is stator resistance
    - Ls is lumped phase inductance

    The solver uses explicit Euler integration and is intentionally modular:
    - back-EMF is injected via a strategy object
    - the integration rule is isolated in EulerIntegrator
    """

    def __init__(
        self,
        parameters: MotorParameters,
        back_emf_model: BackEMFModel | None = None,
        integrator: EulerIntegrator | None = None,
    ) -> None:
        parameters.validate()
        self.parameters = parameters
        self.back_emf_model = back_emf_model or SineBackEMFModel()
        self.integrator = integrator or EulerIntegrator()
        self.state = ElectricalState()

    def reset(self, state: ElectricalState | None = None) -> None:
        if state is None:
            self.state = ElectricalState()
            return
        state.validate()
        self.state = state

    def step(self, inputs: ElectricalInputs, dt: float) -> ElectricalOutputs:
        inputs.validate()
        self.state.validate()

        ea, eb, ec = self.back_emf_model.phase_emf(inputs.RotorAngle, inputs.Speed, self.parameters)
        inductance = self._phase_inductance()
        if inductance <= 0.0:
            raise ElectricalModelError("phase inductance must be greater than 0")

        dia_dt = (inputs.Va - self.parameters.Rs * self.state.Ia - ea) / inductance
        dib_dt = (inputs.Vb - self.parameters.Rs * self.state.Ib - eb) / inductance
        dic_dt = (inputs.Vc - self.parameters.Rs * self.state.Ic - ec) / inductance

        ia = self.integrator.step(self.state.Ia, dia_dt, dt)
        ib = self.integrator.step(self.state.Ib, dib_dt, dt)
        ic = self.integrator.step(self.state.Ic, dic_dt, dt)

        self.state = ElectricalState(Ia=ia, Ib=ib, Ic=ic)
        return ElectricalOutputs(Ia=ia, Ib=ib, Ic=ic)

    def _phase_inductance(self) -> float:
        # Use the average of d- and q-axis inductances as a lumped phase inductance.
        return 0.5 * (self.parameters.Ld + self.parameters.Lq)

    def compute_back_emf(self, rotor_angle: float, speed: float) -> tuple[float, float, float]:
        return self.back_emf_model.phase_emf(rotor_angle, speed, self.parameters)

    def validate(self) -> None:
        self.parameters.validate()
        self.state.validate()

