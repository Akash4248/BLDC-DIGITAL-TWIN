from __future__ import annotations

import pytest

from backend.app.models.inverter_model import (
    DeadTimeCompensationStrategy,
    InverterInputs,
    InverterModel,
    ModulationMethod,
)


class ZeroOffsetCompensator:
    def compensate(self, duty_a: float, duty_b: float, duty_c: float, vdc: float) -> tuple[float, float, float]:
        return duty_a, duty_b, duty_c


def test_sinusoidal_pwm_voltage_mapping() -> None:
    model = InverterModel(modulation=ModulationMethod.SINUSOIDAL_PWM)
    outputs = model.compute(InverterInputs(DutyA=0.75, DutyB=0.25, DutyC=0.5, Vdc=24.0))

    assert outputs.Va == pytest.approx(6.0)
    assert outputs.Vb == pytest.approx(-6.0)
    assert outputs.Vc == pytest.approx(0.0)


def test_svpwm_removes_common_mode() -> None:
    model = InverterModel(modulation=ModulationMethod.SVPWM)
    outputs = model.compute(InverterInputs(DutyA=0.75, DutyB=0.25, DutyC=0.5, Vdc=24.0))

    assert outputs.Va == pytest.approx(6.0)
    assert outputs.Vb == pytest.approx(-6.0)
    assert outputs.Vc == pytest.approx(0.0)
    assert outputs.Va + outputs.Vb + outputs.Vc == pytest.approx(0.0)


def test_dead_time_compensation_hook_is_used() -> None:
    class ShiftCompensator:
        def compensate(self, duty_a: float, duty_b: float, duty_c: float, vdc: float) -> tuple[float, float, float]:
            return duty_a - 0.01, duty_b, duty_c

    model = InverterModel(
        modulation=ModulationMethod.SINUSOIDAL_PWM,
        dead_time_compensator=ShiftCompensator(),
    )
    outputs = model.compute(InverterInputs(DutyA=0.75, DutyB=0.25, DutyC=0.5, Vdc=24.0))

    assert outputs.Va == pytest.approx(5.76)


def test_invalid_duty_rejected() -> None:
    model = InverterModel()
    with pytest.raises(Exception):
        model.compute(InverterInputs(DutyA=1.2, DutyB=0.2, DutyC=0.3, Vdc=24.0))

