from __future__ import annotations

from dataclasses import dataclass
from math import isfinite, pi, sin, sqrt
import random

from backend.app.models.current_sensor_model import (
    CurrentSensorConfig,
    CurrentSensorInputs,
    CurrentSensorModel,
    CurrentSensorOutputs,
)
from backend.app.models.electrical_model import ElectricalInputs, ElectricalModel, ElectricalOutputs
from backend.app.models.encoder_model import EncoderConfig, EncoderInputs, EncoderModel, EncoderOutputs
from backend.app.models.hall_sensor_model import HallSensorConfig, HallSensorInputs, HallSensorModel, HallSensorOutputs
from backend.app.models.inverter_model import InverterInputs, InverterModel
from backend.app.models.mechanical_model import MechanicalInputs, MechanicalModel, MechanicalOutputs
from backend.app.models.motor_parameters import MotorParameters
from backend.app.serial.protocol import TelemetryPacket, TwinStatePacket


class TwinEngineError(ValueError):
    """Raised when the twin engine configuration or inputs are invalid."""


@dataclass(frozen=True)
class TwinEngineConfig:
    SampleRateHz: int = 1000
    HallTransitionTime: float = 0.0
    EncoderTransitionTime: float = 0.0
    CurrentFilterAlpha: float = 1.0
    SensorNoiseStdDev: float = 0.0
    EncoderPPR: int = 2048
    HallElectricalOffset: float = 0.0
    EncoderElectricalOffset: float = 0.0
    EncoderIndexWidthRad: float = 0.0
    RandomSeed: int = 1

    def validate(self) -> None:
        if not isinstance(self.SampleRateHz, int):
            raise TwinEngineError("SampleRateHz must be an integer")
        if self.SampleRateHz <= 0:
            raise TwinEngineError("SampleRateHz must be greater than 0")
        if not isinstance(self.EncoderPPR, int):
            raise TwinEngineError("EncoderPPR must be an integer")
        if self.EncoderPPR <= 0:
            raise TwinEngineError("EncoderPPR must be greater than 0")
        for name, value in (
            ("HallTransitionTime", self.HallTransitionTime),
            ("EncoderTransitionTime", self.EncoderTransitionTime),
            ("CurrentFilterAlpha", self.CurrentFilterAlpha),
            ("SensorNoiseStdDev", self.SensorNoiseStdDev),
            ("HallElectricalOffset", self.HallElectricalOffset),
            ("EncoderElectricalOffset", self.EncoderElectricalOffset),
            ("EncoderIndexWidthRad", self.EncoderIndexWidthRad),
        ):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise TwinEngineError(f"{name} must be a finite number")
        if not 0.0 <= float(self.CurrentFilterAlpha) <= 1.0:
            raise TwinEngineError("CurrentFilterAlpha must be in the range [0, 1]")
        if float(self.HallTransitionTime) < 0.0:
            raise TwinEngineError("HallTransitionTime must be greater than or equal to 0")
        if float(self.EncoderTransitionTime) < 0.0:
            raise TwinEngineError("EncoderTransitionTime must be greater than or equal to 0")
        if float(self.SensorNoiseStdDev) < 0.0:
            raise TwinEngineError("SensorNoiseStdDev must be greater than or equal to 0")
        if float(self.EncoderIndexWidthRad) < 0.0:
            raise TwinEngineError("EncoderIndexWidthRad must be greater than or equal to 0")


@dataclass(frozen=True)
class TwinStepResult:
    telemetry: TelemetryPacket
    inverter: object
    electrical: ElectricalOutputs
    mechanical: MechanicalOutputs
    hall: HallSensorOutputs
    encoder: EncoderOutputs
    current_sensors: CurrentSensorOutputs
    feedback: TwinStatePacket
    electromagnetic_torque: float


class TorqueEstimator:
    """
    Phase-current to torque estimator.

    For a sinusoidal PMSM approximation:

        i_q = (2/3) * [ia*sin(theta_e) + ib*sin(theta_e - 2*pi/3) + ic*sin(theta_e + 2*pi/3)]
        T_e = Kt * i_q

    This keeps the mechanical model independent from the electrical model's
    internal state and makes the torque path deterministic.
    """

    def estimate(self, ia: float, ib: float, ic: float, electrical_angle: float, parameters: MotorParameters) -> float:
        if not all(isfinite(float(v)) for v in (ia, ib, ic, electrical_angle)):
            raise TwinEngineError("currents and angle must be finite")
        iq = (2.0 / 3.0) * (
            ia * sin(electrical_angle)
            + ib * sin(electrical_angle - 2.0 * pi / 3.0)
            + ic * sin(electrical_angle + 2.0 * pi / 3.0)
        )
        return parameters.Kt * iq


class TwinSimulationEngine:
    """
    Deterministic 1000 Hz simulation pipeline.

    Execution order:

        Receive USB -> Inverter -> Electrical Model -> Mechanical Model ->
        Hall -> Encoder -> Current Sensor -> Send Feedback

    The engine is deterministic because:
    - it runs at a fixed dt derived from the configured sample rate
    - it does not depend on wall-clock time
    - sensor noise is seeded
    - every submodule is stepped exactly once per tick
    """

    def __init__(
        self,
        parameters: MotorParameters,
        inverter_model: InverterModel,
        electrical_model: ElectricalModel,
        mechanical_model: MechanicalModel,
        hall_model: HallSensorModel | None = None,
        encoder_model: EncoderModel | None = None,
        current_sensor_model: CurrentSensorModel | None = None,
        torque_estimator: TorqueEstimator | None = None,
        config: TwinEngineConfig | None = None,
    ) -> None:
        self.parameters = parameters
        self.parameters.validate()
        self.config = config or TwinEngineConfig()
        self.config.validate()

        self.dt_s = 1.0 / float(self.config.SampleRateHz)
        self.inverter_model = inverter_model
        self.electrical_model = electrical_model
        self.mechanical_model = mechanical_model
        self.hall_model = hall_model or HallSensorModel(
            HallSensorConfig(
                ElectricalOffset=self.config.HallElectricalOffset,
                TransitionTime=self.config.HallTransitionTime,
            )
        )
        self.encoder_model = encoder_model or EncoderModel(
            EncoderConfig(
                PPR=self.config.EncoderPPR,
                ElectricalOffset=self.config.EncoderElectricalOffset,
                IndexWidthRad=self.config.EncoderIndexWidthRad,
                TransitionTime=self.config.EncoderTransitionTime,
            )
        )
        self.current_sensor_model = current_sensor_model or CurrentSensorModel(
            CurrentSensorConfig(
                NoiseStdDev=self.config.SensorNoiseStdDev,
                FilterAlpha=self.config.CurrentFilterAlpha,
            ),
            rng=random.Random(self.config.RandomSeed),
            seed=self.config.RandomSeed,
        )
        self.torque_estimator = torque_estimator or TorqueEstimator()
        self.step_index = 0

    def reset(self) -> None:
        self.electrical_model.reset()
        self.mechanical_model.reset()
        self.hall_model.reset()
        self.encoder_model.reset()
        self.current_sensor_model.reset(seed=self.config.RandomSeed)
        self.step_index = 0

    def step(self, telemetry: TelemetryPacket) -> TwinStepResult:
        self._validate_telemetry(telemetry)

        inverter_inputs = InverterInputs(
            DutyA=telemetry.duty_a,
            DutyB=telemetry.duty_b,
            DutyC=telemetry.duty_c,
            Vdc=telemetry.vdc,
        )
        inverter_outputs = self.inverter_model.compute(inverter_inputs)

        electrical_inputs = ElectricalInputs(
            Va=inverter_outputs.Va,
            Vb=inverter_outputs.Vb,
            Vc=inverter_outputs.Vc,
            RotorAngle=self.mechanical_model.state.MechanicalAngle,
            Speed=self.mechanical_model.state.Speed,
        )
        electrical_outputs = self.electrical_model.step(electrical_inputs, self.dt_s)

        electrical_angle = self.mechanical_model.parameters.PolePairs * self.mechanical_model.state.MechanicalAngle
        electromagnetic_torque = self.torque_estimator.estimate(
            electrical_outputs.Ia,
            electrical_outputs.Ib,
            electrical_outputs.Ic,
            electrical_angle,
            self.parameters,
        )

        mechanical_outputs = self.mechanical_model.step(
            MechanicalInputs(Torque=electromagnetic_torque, LoadTorque=self.parameters.LoadTorque),
            self.dt_s,
        )

        hall_outputs = self.hall_model.step(
            HallSensorInputs(ElectricalAngle=mechanical_outputs.ElectricalAngle),
            self.dt_s,
        )

        encoder_outputs = self.encoder_model.step(
            EncoderInputs(MechanicalAngle=mechanical_outputs.MechanicalAngle),
            self.dt_s,
        )

        current_sensor_outputs = self.current_sensor_model.step(
            CurrentSensorInputs(
                Ia=electrical_outputs.Ia,
                Ib=electrical_outputs.Ib,
                Ic=electrical_outputs.Ic,
            )
        )

        feedback = TwinStatePacket(
            sequence=telemetry.sequence,
            ia=electrical_outputs.Ia,
            ib=electrical_outputs.Ib,
            ic=electrical_outputs.Ic,
            rotor_angle=mechanical_outputs.ElectricalAngle,
            rotor_speed=mechanical_outputs.Speed,
        )

        self.step_index += 1
        return TwinStepResult(
            telemetry=telemetry,
            inverter=inverter_outputs,
            electrical=electrical_outputs,
            mechanical=mechanical_outputs,
            hall=hall_outputs,
            encoder=encoder_outputs,
            current_sensors=current_sensor_outputs,
            feedback=feedback,
            electromagnetic_torque=electromagnetic_torque,
        )

    @staticmethod
    def _validate_telemetry(telemetry: TelemetryPacket) -> None:
        if not isinstance(telemetry.timestamp_us, int):
            raise TwinEngineError("telemetry timestamp must be an integer")
        for name, value in (
            ("duty_a", telemetry.duty_a),
            ("duty_b", telemetry.duty_b),
            ("duty_c", telemetry.duty_c),
            ("vdc", telemetry.vdc),
        ):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise TwinEngineError(f"{name} must be a finite number")
