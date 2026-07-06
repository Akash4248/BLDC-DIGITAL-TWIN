from __future__ import annotations

from backend.app.models.current_sensor_model import CurrentSensorConfig, CurrentSensorModel
from backend.app.models.electrical_model import ElectricalModel
from backend.app.models.encoder_model import EncoderConfig, EncoderModel
from backend.app.models.inverter_model import InverterModel, ModulationMethod
from backend.app.models.hall_sensor_model import HallSensorModel
from backend.app.models.mechanical_model import MechanicalModel
from backend.app.models.motor_parameters import MotorParameters
from backend.app.serial.protocol import TelemetryPacket
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


def test_deterministic_single_step_pipeline() -> None:
    params = make_params()
    engine = TwinSimulationEngine(
        parameters=params,
        inverter_model=InverterModel(ModulationMethod.SVPWM),
        electrical_model=ElectricalModel(params),
        mechanical_model=MechanicalModel(params),
        hall_model=HallSensorModel(),
        encoder_model=EncoderModel(config=EncoderConfig(PPR=1024)),
        current_sensor_model=CurrentSensorModel(CurrentSensorConfig()),
        config=TwinEngineConfig(SampleRateHz=1000, RandomSeed=1),
    )

    telemetry = TelemetryPacket(
        sequence=1,
        timestamp_us=1000,
        duty_a=0.5,
        duty_b=0.5,
        duty_c=0.5,
        vdc=24.0,
    )
    result_1 = engine.step(telemetry)
    engine.reset()
    result_2 = engine.step(telemetry)

    assert result_1.feedback == result_2.feedback
    assert result_1.hall == result_2.hall
    assert result_1.encoder == result_2.encoder
    assert result_1.current_sensors == result_2.current_sensors


def test_engine_uses_fixed_dt() -> None:
    params = make_params()
    engine = TwinSimulationEngine(
        parameters=params,
        inverter_model=InverterModel(),
        electrical_model=ElectricalModel(params),
        mechanical_model=MechanicalModel(params),
        config=TwinEngineConfig(SampleRateHz=1000),
    )

    assert engine.dt_s == 0.001
