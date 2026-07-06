from __future__ import annotations

import pytest

from backend.app.models.current_sensor_model import CurrentSensorConfig, CurrentSensorModel
from backend.app.models.electrical_model import ElectricalModel
from backend.app.models.encoder_model import EncoderConfig, EncoderModel
from backend.app.models.hall_sensor_model import HallSensorModel
from backend.app.models.inverter_model import InverterModel
from backend.app.models.mechanical_model import MechanicalModel
from backend.app.models.motor_parameters import MotorParameters
from backend.app.serial.protocol import TelemetryPacket, encode_telemetry, decode_frame, TwinStatePacket
from backend.app.schemas.simulation import FaultInjectionRequest, FaultType, TelemetryStepRequest
from backend.app.services.faults import FaultManager, FaultType as ServiceFaultType
from backend.app.services.simulation_service import SimulationService, SimulationServiceError
from backend.app.twin.engine import TwinEngineConfig, TwinSimulationEngine


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


def make_service() -> SimulationService:
    params = make_params()
    engine = TwinSimulationEngine(
        parameters=params,
        inverter_model=InverterModel(),
        electrical_model=ElectricalModel(params),
        mechanical_model=MechanicalModel(params),
        hall_model=HallSensorModel(),
        encoder_model=EncoderModel(EncoderConfig(PPR=1024)),
        current_sensor_model=CurrentSensorModel(CurrentSensorConfig()),
        config=TwinEngineConfig(SampleRateHz=1000, RandomSeed=1),
    )
    return SimulationService(engine)


def test_service_control_and_parameter_update() -> None:
    service = make_service()
    service.start()
    assert service.running is True

    updated = service.update_parameters(
        MotorParameters(
            Rs=0.6,
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
    )
    assert updated["Rs"] == pytest.approx(0.6)


def test_service_step_and_fault_injection() -> None:
    service = make_service()
    service.start()

    result = service.step_from_telemetry(
        TelemetryStepRequest(
            sequence=1,
            timestamp_us=1000,
            duty_a=0.5,
            duty_b=0.5,
            duty_c=0.5,
            vdc=24.0,
        )
    )
    assert isinstance(result["feedback"], TwinStatePacket)

    service.inject_fault(
        FaultInjectionRequest(fault_type=FaultType.current_sensor_offset, enabled=True, value=10.0)
    )
    result_with_fault = service.step_from_telemetry(
        TelemetryStepRequest(
            sequence=2,
            timestamp_us=2000,
            duty_a=0.5,
            duty_b=0.5,
            duty_c=0.5,
            vdc=24.0,
        )
    )
    assert result_with_fault["current_sensors"].ADCA >= result["current_sensors"].ADCA


def test_service_applies_low_voltage_and_high_load_faults() -> None:
    service = make_service()
    service.start()

    service.inject_fault(FaultInjectionRequest(fault_type=FaultType.low_voltage, enabled=True, value=0.5))
    service.inject_fault(FaultInjectionRequest(fault_type=FaultType.high_load, enabled=True, value=3.0))

    result = service.step_from_telemetry(
        TelemetryStepRequest(
            sequence=1,
            timestamp_us=1000,
            duty_a=0.5,
            duty_b=0.5,
            duty_c=0.5,
            vdc=24.0,
        )
    )

    assert result["telemetry"].vdc == pytest.approx(12.0)
    assert result["mechanical"].Speed <= 0.0 or result["mechanical"].Speed < 10.0


def test_service_rejects_step_when_stopped() -> None:
    service = make_service()
    with pytest.raises(SimulationServiceError):
        service.step_from_telemetry(
            TelemetryStepRequest(
                sequence=1,
                timestamp_us=1000,
                duty_a=0.5,
                duty_b=0.5,
                duty_c=0.5,
                vdc=24.0,
            )
        )


def test_fault_manager_freezes_outputs() -> None:
    params = make_params()
    engine = TwinSimulationEngine(
        parameters=params,
        inverter_model=InverterModel(),
        electrical_model=ElectricalModel(params),
        mechanical_model=MechanicalModel(params),
        hall_model=HallSensorModel(),
        encoder_model=EncoderModel(EncoderConfig(PPR=1024)),
        current_sensor_model=CurrentSensorModel(CurrentSensorConfig()),
        config=TwinEngineConfig(SampleRateHz=1000, RandomSeed=1),
    )
    service = SimulationService(engine)
    service.start()
    service.inject_fault(FaultInjectionRequest(fault_type=FaultType.hall_failure, enabled=True))
    first = service.step_from_telemetry(
        TelemetryStepRequest(
            sequence=1,
            timestamp_us=1000,
            duty_a=0.5,
            duty_b=0.5,
            duty_c=0.5,
            vdc=24.0,
        )
    )["hall"]
    second = service.step_from_telemetry(
        TelemetryStepRequest(
            sequence=2,
            timestamp_us=2000,
            duty_a=0.5,
            duty_b=0.5,
            duty_c=0.5,
            vdc=24.0,
        )
    )["hall"]
    assert first == second
