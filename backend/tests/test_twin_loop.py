from __future__ import annotations

from backend.app.models.current_sensor_model import CurrentSensorConfig, CurrentSensorModel
from backend.app.models.electrical_model import ElectricalModel
from backend.app.models.encoder_model import EncoderConfig, EncoderModel
from backend.app.models.hall_sensor_model import HallSensorModel
from backend.app.models.inverter_model import InverterModel
from backend.app.models.mechanical_model import MechanicalModel
from backend.app.models.motor_parameters import MotorParameters
from backend.app.serial.protocol import TelemetryPacket, decode_frame
from backend.app.services.twin_loop import TwinUsbLoop
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


def test_usb_loop_processes_binary_frames() -> None:
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
    loop = TwinUsbLoop(engine)
    telemetry = TelemetryPacket(
        sequence=9,
        timestamp_us=1234,
        duty_a=0.5,
        duty_b=0.5,
        duty_c=0.5,
        vdc=24.0,
    )

    from backend.app.serial.protocol import encode_telemetry

    result = loop.process(encode_telemetry(telemetry))
    packet = decode_frame(result.feedback_frame)

    assert packet.sequence == telemetry.sequence
    assert result.feedback_packet == packet

