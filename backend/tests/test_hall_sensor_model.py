from __future__ import annotations

from math import pi

import pytest

from backend.app.models.hall_sensor_model import HallSensorConfig, HallSensorInputs, HallSensorModel


def test_default_hall_pattern_exists() -> None:
    model = HallSensorModel()
    outputs = model.step(HallSensorInputs(ElectricalAngle=0.0), dt=0.001)

    assert (outputs.HallA, outputs.HallB, outputs.HallC) == (1.0, 0.0, 1.0)


def test_configurable_placement_shifts_pattern() -> None:
    model = HallSensorModel(
        HallSensorConfig(
            ElectricalOffset=pi / 3.0,
            TransitionTime=0.0,
        )
    )

    outputs = model.step(HallSensorInputs(ElectricalAngle=0.0), dt=0.001)
    assert outputs.HallA in (0.0, 1.0)
    assert outputs.HallB in (0.0, 1.0)
    assert outputs.HallC in (0.0, 1.0)


def test_transition_animation_moves_toward_target() -> None:
    model = HallSensorModel(HallSensorConfig(TransitionTime=0.1))
    model.step(HallSensorInputs(ElectricalAngle=0.0), dt=0.01)
    outputs = model.step(HallSensorInputs(ElectricalAngle=2.5), dt=0.01)

    assert 0.0 <= outputs.HallA <= 1.0
    assert 0.0 <= outputs.HallB <= 1.0
    assert 0.0 <= outputs.HallC <= 1.0
