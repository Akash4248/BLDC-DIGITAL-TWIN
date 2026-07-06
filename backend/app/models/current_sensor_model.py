from __future__ import annotations

from dataclasses import dataclass
from math import isfinite
import random


class CurrentSensorModelError(ValueError):
    """Raised when current sensor inputs or configuration are invalid."""


@dataclass(frozen=True)
class CurrentSensorConfig:
    """
    Current sensor emulation configuration.

    Gain, offset, noise, and filtering are all modeled in ADC space.
    """

    ADCResolution: int = 4096
    ADCReferenceVoltage: float = 3.3
    Sensitivity: float = 1.0
    OffsetA: float = 0.0
    OffsetB: float = 0.0
    OffsetC: float = 0.0
    NoiseStdDev: float = 0.0
    FilterAlpha: float = 1.0

    def validate(self) -> None:
        if not isinstance(self.ADCResolution, int):
            raise CurrentSensorModelError("ADCResolution must be an integer")
        if self.ADCResolution <= 0:
            raise CurrentSensorModelError("ADCResolution must be greater than 0")
        for name, value in (
            ("ADCReferenceVoltage", self.ADCReferenceVoltage),
            ("Sensitivity", self.Sensitivity),
            ("OffsetA", self.OffsetA),
            ("OffsetB", self.OffsetB),
            ("OffsetC", self.OffsetC),
            ("NoiseStdDev", self.NoiseStdDev),
            ("FilterAlpha", self.FilterAlpha),
        ):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise CurrentSensorModelError(f"{name} must be a finite number")
        if float(self.ADCReferenceVoltage) <= 0.0:
            raise CurrentSensorModelError("ADCReferenceVoltage must be greater than 0")
        if float(self.Sensitivity) <= 0.0:
            raise CurrentSensorModelError("Sensitivity must be greater than 0")
        if float(self.NoiseStdDev) < 0.0:
            raise CurrentSensorModelError("NoiseStdDev must be greater than or equal to 0")
        if not 0.0 <= float(self.FilterAlpha) <= 1.0:
            raise CurrentSensorModelError("FilterAlpha must be in the range [0, 1]")


@dataclass(frozen=True)
class CurrentSensorInputs:
    Ia: float
    Ib: float
    Ic: float

    def validate(self) -> None:
        for name, value in (("Ia", self.Ia), ("Ib", self.Ib), ("Ic", self.Ic)):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise CurrentSensorModelError(f"{name} must be a finite number")


@dataclass(frozen=True)
class CurrentSensorOutputs:
    ADCA: int
    ADCB: int
    ADCC: int


@dataclass
class CurrentSensorState:
    ADCA: float = 0.0
    ADCB: float = 0.0
    ADCC: float = 0.0

    def validate(self) -> None:
        for name, value in (("ADCA", self.ADCA), ("ADCB", self.ADCB), ("ADCC", self.ADCC)):
            if not isinstance(value, (int, float)) or not isfinite(float(value)):
                raise CurrentSensorModelError(f"{name} must be a finite number")


class CurrentSensorModel:
    """
    Phase current to ADC emulation.

    ADC conversion:

        v_sensor = gain * i + offset + noise
        adc = round((v_sensor / Vref) * (ADCResolution - 1))

    Filtering is a first-order IIR filter in ADC units:

        y[k] = alpha * x[k] + (1 - alpha) * y[k-1]

    Where alpha is FilterAlpha.
    """

    def __init__(
        self,
        config: CurrentSensorConfig,
        rng: random.Random | None = None,
        seed: int | None = None,
    ) -> None:
        self.config = config
        self.config.validate()
        self._seed = seed if seed is not None else 0
        self.rng = rng or random.Random(self._seed)
        self.state = CurrentSensorState()

    def reset(self, state: CurrentSensorState | None = None, seed: int | None = None) -> None:
        if seed is not None:
            self._seed = seed
            self.rng.seed(self._seed)
        else:
            self.rng.seed(self._seed)
        if state is None:
            self.state = CurrentSensorState()
            return
        state.validate()
        self.state = state

    def step(self, inputs: CurrentSensorInputs) -> CurrentSensorOutputs:
        inputs.validate()
        self.config.validate()
        self.state.validate()

        raw_a = self._current_to_adc(inputs.Ia, self.config.OffsetA)
        raw_b = self._current_to_adc(inputs.Ib, self.config.OffsetB)
        raw_c = self._current_to_adc(inputs.Ic, self.config.OffsetC)

        filtered_a = self._filter(self.state.ADCA, raw_a)
        filtered_b = self._filter(self.state.ADCB, raw_b)
        filtered_c = self._filter(self.state.ADCC, raw_c)

        self.state = CurrentSensorState(filtered_a, filtered_b, filtered_c)
        return CurrentSensorOutputs(
            ADCA=self._to_adc_code(filtered_a),
            ADCB=self._to_adc_code(filtered_b),
            ADCC=self._to_adc_code(filtered_c),
        )

    def _current_to_adc(self, current: float, offset: float) -> float:
        noise = self.rng.gauss(0.0, self.config.NoiseStdDev) if self.config.NoiseStdDev > 0.0 else 0.0
        voltage = self.config.Sensitivity * float(current) + offset + noise
        adc = (voltage / self.config.ADCReferenceVoltage) * (self.config.ADCResolution - 1)
        return self._clamp(adc, 0.0, float(self.config.ADCResolution - 1))

    def _filter(self, previous: float, sample: float) -> float:
        alpha = self.config.FilterAlpha
        return alpha * sample + (1.0 - alpha) * previous

    def _to_adc_code(self, value: float) -> int:
        return int(round(self._clamp(value, 0.0, float(self.config.ADCResolution - 1))))

    @staticmethod
    def _clamp(value: float, minimum: float, maximum: float) -> float:
        return max(minimum, min(maximum, float(value)))
