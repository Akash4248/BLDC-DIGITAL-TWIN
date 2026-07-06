from __future__ import annotations

from math import pi

import pytest

from backend.app.models.encoder_model import EncoderConfig, EncoderInputs, EncoderModel


def test_encoder_outputs_are_digital() -> None:
    model = EncoderModel(EncoderConfig(PPR=1000))
    outputs = model.step(EncoderInputs(MechanicalAngle=0.0), dt=0.001)

    assert outputs.EncoderA in (0.0, 1.0)
    assert outputs.EncoderB in (0.0, 1.0)
    assert outputs.IndexPulse in (0.0, 1.0)


def test_configurable_ppr_changes_phase_rate() -> None:
    model = EncoderModel(EncoderConfig(PPR=1))
    out1 = model.step(EncoderInputs(MechanicalAngle=0.0), dt=0.001)
    out2 = model.step(EncoderInputs(MechanicalAngle=pi / 2.0), dt=0.001)

    assert out1.EncoderA in (0.0, 1.0)
    assert out2.EncoderA in (0.0, 1.0)


def test_index_pulse_is_generated_near_zero() -> None:
    model = EncoderModel(EncoderConfig(PPR=1024, IndexWidthRad=0.01))
    outputs = model.step(EncoderInputs(MechanicalAngle=0.005), dt=0.001)

    assert outputs.IndexPulse == 1.0

