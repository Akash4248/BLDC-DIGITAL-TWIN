from __future__ import annotations

import pytest

from backend.app.models.electrical_model import (
    ElectricalInputs,
    ElectricalModel,
    ElectricalState,
    SineBackEMFModel,
)
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
        RotorInertia=0.00002,
        Friction=0.0001,
        LoadTorque=0.0,
        Vdc=24.0,
    )


def test_back_emf_uses_electrical_angle_conversion() -> None:
    model = ElectricalModel(make_params(), back_emf_model=SineBackEMFModel())
    ea, eb, ec = model.compute_back_emf(rotor_angle=0.0, speed=100.0)

    assert ea == pytest.approx(0.0)
    assert eb == pytest.approx(-8.6602540378, rel=1e-6)
    assert ec == pytest.approx(8.6602540378, rel=1e-6)


def test_euler_integration_updates_currents() -> None:
    model = ElectricalModel(make_params())
    model.reset(ElectricalState(Ia=0.0, Ib=0.0, Ic=0.0))

    outputs = model.step(
        ElectricalInputs(
            Va=10.0,
            Vb=0.0,
            Vc=0.0,
            RotorAngle=0.0,
            Speed=0.0,
        ),
        dt=0.001,
    )

    assert outputs.Ia == pytest.approx(10.0)
    assert outputs.Ib == pytest.approx(0.0)
    assert outputs.Ic == pytest.approx(0.0)


def test_solver_is_stateful() -> None:
    model = ElectricalModel(make_params())

    first = model.step(
        ElectricalInputs(Va=10.0, Vb=0.0, Vc=0.0, RotorAngle=0.0, Speed=0.0),
        dt=0.001,
    )
    second = model.step(
        ElectricalInputs(Va=10.0, Vb=0.0, Vc=0.0, RotorAngle=0.0, Speed=0.0),
        dt=0.001,
    )

    assert second.Ia > first.Ia

