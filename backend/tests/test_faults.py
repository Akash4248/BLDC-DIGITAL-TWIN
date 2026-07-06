from __future__ import annotations

import pytest

from backend.app.models.current_sensor_model import CurrentSensorOutputs
from backend.app.models.electrical_model import ElectricalOutputs
from backend.app.models.encoder_model import EncoderOutputs
from backend.app.models.hall_sensor_model import HallSensorOutputs
from backend.app.models.inverter_model import InverterOutputs
from backend.app.models.mechanical_model import MechanicalOutputs
from backend.app.serial.protocol import TelemetryPacket, TwinStatePacket
from backend.app.schemas.simulation import FaultInjectionRequest, FaultType
from backend.app.services.faults import FaultManager, FaultType as ServiceFaultType
from backend.app.twin.engine import TwinStepResult


def make_result() -> TwinStepResult:
    return TwinStepResult(
        telemetry=TelemetryPacket(
            sequence=1,
            timestamp_us=1000,
            duty_a=0.6,
            duty_b=0.4,
            duty_c=0.5,
            vdc=24.0,
        ),
        inverter=InverterOutputs(Va=1.0, Vb=2.0, Vc=3.0),
        electrical=ElectricalOutputs(Ia=1.0, Ib=2.0, Ic=3.0),
        mechanical=MechanicalOutputs(Speed=100.0, MechanicalAngle=0.25, ElectricalAngle=1.0),
        hall=HallSensorOutputs(HallA=1.0, HallB=0.0, HallC=1.0),
        encoder=EncoderOutputs(EncoderA=1.0, EncoderB=0.0, IndexPulse=1.0),
        current_sensors=CurrentSensorOutputs(ADCA=100, ADCB=200, ADCC=300),
        feedback=TwinStatePacket(
            sequence=1,
            ia=1.0,
            ib=2.0,
            ic=3.0,
            rotor_angle=1.0,
            rotor_speed=100.0,
        ),
        electromagnetic_torque=2.0,
    )


def test_open_phase_sets_one_duty_low() -> None:
    faults = FaultManager()
    faults.upsert(ServiceFaultType.open_phase, enabled=True, value=1)

    telemetry = faults.apply_telemetry(make_result().telemetry)
    assert telemetry.duty_b == pytest.approx(0.0)
    assert telemetry.duty_a == pytest.approx(0.6)


def test_short_circuit_forces_equal_duties() -> None:
    faults = FaultManager()
    faults.upsert(ServiceFaultType.short_circuit, enabled=True)

    telemetry = faults.apply_telemetry(make_result().telemetry)
    assert telemetry.duty_a == pytest.approx(0.5)
    assert telemetry.duty_b == pytest.approx(0.5)
    assert telemetry.duty_c == pytest.approx(0.5)


def test_low_voltage_scales_bus_voltage() -> None:
    faults = FaultManager()
    faults.upsert(ServiceFaultType.low_voltage, enabled=True, value=0.5)

    telemetry = faults.apply_telemetry(make_result().telemetry)
    assert telemetry.vdc == pytest.approx(12.0)


def test_current_sensor_offset_applies_adc_shift() -> None:
    faults = FaultManager()
    faults.upsert(ServiceFaultType.current_sensor_offset, enabled=True, value=25)

    result = faults.apply(make_result())
    assert result.current_sensors.ADCA == 125
    assert result.current_sensors.ADCB == 225
    assert result.current_sensors.ADCC == 325


def test_current_sensor_offset_uses_default_value() -> None:
    faults = FaultManager()
    faults.upsert(ServiceFaultType.current_sensor_offset, enabled=True, value=None)

    result = faults.apply(make_result())
    assert result.current_sensors.ADCA == 140
    assert result.current_sensors.ADCB == 240
    assert result.current_sensors.ADCC == 340


def test_hall_and_encoder_failures_freeze_outputs() -> None:
    faults = FaultManager()
    faults.upsert(ServiceFaultType.hall_failure, enabled=True)
    faults.upsert(ServiceFaultType.encoder_failure, enabled=True)

    first = faults.apply(make_result())
    second = faults.apply(
        TwinStepResult(
            telemetry=make_result().telemetry,
            inverter=make_result().inverter,
            electrical=make_result().electrical,
            mechanical=make_result().mechanical,
            hall=HallSensorOutputs(HallA=0.0, HallB=1.0, HallC=0.0),
            encoder=EncoderOutputs(EncoderA=0.0, EncoderB=1.0, IndexPulse=0.0),
            current_sensors=make_result().current_sensors,
            feedback=make_result().feedback,
            electromagnetic_torque=make_result().electromagnetic_torque,
        )
    )

    assert second.hall == first.hall
    assert second.encoder == first.encoder


def test_locked_rotor_freezes_mechanics() -> None:
    faults = FaultManager()
    faults.upsert(ServiceFaultType.locked_rotor, enabled=True)

    result = faults.apply(make_result())
    assert result.mechanical.Speed == pytest.approx(100.0)
    assert result.feedback.rotor_speed == pytest.approx(0.0)
