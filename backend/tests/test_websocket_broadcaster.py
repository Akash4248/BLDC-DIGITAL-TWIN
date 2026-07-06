from __future__ import annotations

import pytest

from backend.app.models.current_sensor_model import CurrentSensorOutputs
from backend.app.models.electrical_model import ElectricalOutputs
from backend.app.models.encoder_model import EncoderOutputs
from backend.app.models.hall_sensor_model import HallSensorOutputs
from backend.app.models.inverter_model import InverterOutputs
from backend.app.models.mechanical_model import MechanicalOutputs
from backend.app.serial.protocol import TelemetryPacket, TwinStatePacket
from backend.app.services.ws_broadcaster import WebSocketBroadcaster
from backend.app.twin.engine import TwinStepResult


class DummyWebSocket:
    def __init__(self) -> None:
        self.accepted = False
        self.messages: list[dict] = []

    async def accept(self) -> None:
        self.accepted = True

    async def send_json(self, payload):
        self.messages.append(payload)


def make_result() -> TwinStepResult:
    return TwinStepResult(
        telemetry=TelemetryPacket(
            sequence=2,
            timestamp_us=2000,
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
            sequence=2,
            ia=1.0,
            ib=2.0,
            ic=3.0,
            rotor_angle=1.0,
            rotor_speed=100.0,
        ),
        electromagnetic_torque=2.0,
    )


@pytest.mark.anyio
async def test_broadcaster_fans_out_to_multiple_clients() -> None:
    broadcaster = WebSocketBroadcaster()
    ws1 = DummyWebSocket()
    ws2 = DummyWebSocket()

    await broadcaster.connect(ws1)
    await broadcaster.connect(ws2)
    await broadcaster.broadcast(make_result())

    assert ws1.accepted is True
    assert ws2.accepted is True
    assert len(ws1.messages) == 1
    assert len(ws2.messages) == 1
    assert ws1.messages[0]["telemetry"]["duty_a"] == pytest.approx(0.6)


@pytest.fixture
def anyio_backend() -> str:
    return "asyncio"
