from __future__ import annotations

import random

import pytest

from backend.app.models.current_sensor_model import CurrentSensorConfig, CurrentSensorInputs, CurrentSensorModel


def test_current_sensor_emits_adc_codes() -> None:
    model = CurrentSensorModel(CurrentSensorConfig())
    outputs = model.step(CurrentSensorInputs(Ia=0.0, Ib=0.5, Ic=-0.5))

    assert isinstance(outputs.ADCA, int)
    assert isinstance(outputs.ADCB, int)
    assert isinstance(outputs.ADCC, int)


def test_current_sensor_supports_offset_gain_and_filter() -> None:
    model = CurrentSensorModel(
        CurrentSensorConfig(
            Sensitivity=2.0,
            OffsetA=0.1,
            OffsetB=0.1,
            OffsetC=0.1,
            FilterAlpha=0.5,
        ),
        rng=random.Random(0),
    )

    first = model.step(CurrentSensorInputs(Ia=1.0, Ib=1.0, Ic=1.0))
    second = model.step(CurrentSensorInputs(Ia=1.0, Ib=1.0, Ic=1.0))

    assert second.ADCA >= first.ADCA


def test_noise_configuration_is_accepted() -> None:
    model = CurrentSensorModel(
        CurrentSensorConfig(NoiseStdDev=0.01),
        rng=random.Random(0),
    )
    outputs = model.step(CurrentSensorInputs(Ia=1.0, Ib=0.0, Ic=-1.0))

    assert outputs.ADCA >= 0

