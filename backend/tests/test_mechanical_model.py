from __future__ import annotations

import pytest

from backend.app.models.mechanical_model import MechanicalInputs, MechanicalModel, MechanicalState
from backend.app.models.motor_parameters import MotorParameters


def make_params() -> MotorParameters:
    return MotorParameters(
        Rs=0.5,
        Ld=0.001,
        Lq=0.001,
        PolePairs=4,
        FluxLinkage=0.02,
        Kt=0.1,
        Ke=0.1,
        RotorInertia=0.01,
        Friction=0.1,
        LoadTorque=0.0,
        Vdc=24.0,
    )


def test_mechanical_speed_increases_with_net_torque() -> None:
    model = MechanicalModel(make_params())
    model.reset(MechanicalState(Speed=0.0, MechanicalAngle=0.0))

    outputs = model.step(MechanicalInputs(Torque=1.0, LoadTorque=0.0), dt=0.01)

    assert outputs.Speed == pytest.approx(1.0)
    assert outputs.MechanicalAngle == pytest.approx(0.0)
    assert outputs.ElectricalAngle == pytest.approx(0.0)


def test_mechanical_angle_integrates_speed() -> None:
    model = MechanicalModel(make_params())
    model.reset(MechanicalState(Speed=10.0, MechanicalAngle=0.0))

    outputs = model.step(MechanicalInputs(Torque=0.0, LoadTorque=0.0), dt=0.01)

    assert outputs.MechanicalAngle == pytest.approx(0.1)
    assert outputs.ElectricalAngle == pytest.approx(0.4)


def test_load_and_friction_reduce_speed() -> None:
    params = make_params()
    params.Friction = 0.2
    params.LoadTorque = 0.5
    model = MechanicalModel(params)
    model.reset(MechanicalState(Speed=5.0, MechanicalAngle=0.0))

    outputs = model.step(MechanicalInputs(Torque=0.0), dt=0.01)

    assert outputs.Speed < 5.0

