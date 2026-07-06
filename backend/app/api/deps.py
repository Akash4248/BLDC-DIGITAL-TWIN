from __future__ import annotations

from functools import lru_cache

from backend.app.models.current_sensor_model import CurrentSensorConfig, CurrentSensorModel
from backend.app.models.electrical_model import ElectricalModel
from backend.app.models.encoder_model import EncoderConfig, EncoderModel
from backend.app.models.hall_sensor_model import HallSensorModel
from backend.app.models.inverter_model import InverterModel, ModulationMethod
from backend.app.models.mechanical_model import MechanicalModel
from backend.app.models.motor_parameters import MotorParameters
from backend.app.schemas.simulation import MotorParametersUpdate
from backend.app.twin.engine import TwinEngineConfig, TwinSimulationEngine
from backend.app.services.simulation_service import SimulationService


@lru_cache(maxsize=1)
def get_simulation_service() -> SimulationService:
    parameters = MotorParameters(
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
    engine = TwinSimulationEngine(
        parameters=parameters,
        inverter_model=InverterModel(ModulationMethod.SVPWM),
        electrical_model=ElectricalModel(parameters),
        mechanical_model=MechanicalModel(parameters),
        hall_model=HallSensorModel(),
        encoder_model=EncoderModel(EncoderConfig(PPR=2048)),
        current_sensor_model=CurrentSensorModel(CurrentSensorConfig()),
        config=TwinEngineConfig(SampleRateHz=1000, RandomSeed=1),
    )
    return SimulationService(engine)

