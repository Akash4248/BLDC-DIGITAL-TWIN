from __future__ import annotations

from pathlib import Path

import pytest

from backend.app.models.motor_parameters import MotorParameterError, MotorParameters
from backend.app.services.motor_parameter_store import MotorParameterStore


def test_validation_accepts_valid_parameters() -> None:
    params = MotorParameters(
        Rs=0.8,
        Ld=0.0012,
        Lq=0.0013,
        PolePairs=7,
        FluxLinkage=0.024,
        Kt=0.12,
        Ke=0.12,
        RotorInertia=0.00001,
        Friction=0.0002,
        LoadTorque=0.1,
        Vdc=24.0,
    )

    assert params.PolePairs == 7


def test_validation_rejects_negative_resistance() -> None:
    with pytest.raises(MotorParameterError):
        MotorParameters(
            Rs=-0.1,
            Ld=0.0012,
            Lq=0.0013,
            PolePairs=7,
            FluxLinkage=0.024,
            Kt=0.12,
            Ke=0.12,
            RotorInertia=0.00001,
            Friction=0.0002,
            LoadTorque=0.1,
            Vdc=24.0,
        )


def test_round_trip_json(tmp_path: Path) -> None:
    params = MotorParameters(
        Rs=0.5,
        Ld=0.001,
        Lq=0.0011,
        PolePairs=4,
        FluxLinkage=0.02,
        Kt=0.11,
        Ke=0.11,
        RotorInertia=0.00002,
        Friction=0.0003,
        LoadTorque=0.2,
        Vdc=48.0,
    )

    path = tmp_path / "motor_parameters.json"
    params.save_json(path)
    loaded = MotorParameters.load_json(path)

    assert loaded.to_dict() == params.to_dict()


def test_runtime_update_and_store_persistence(tmp_path: Path) -> None:
    path = tmp_path / "params.json"
    store = MotorParameterStore(path)

    updated = store.update(
        Rs=0.7,
        Ld=0.001,
        Lq=0.001,
        PolePairs=6,
        FluxLinkage=0.03,
        Kt=0.13,
        Ke=0.13,
        RotorInertia=0.00003,
        Friction=0.0004,
        LoadTorque=0.15,
        Vdc=36.0,
    )

    assert updated.Rs == pytest.approx(0.7)
    assert path.exists()

    reloaded = MotorParameterStore(path).load()
    assert reloaded.to_dict() == updated.to_dict()

