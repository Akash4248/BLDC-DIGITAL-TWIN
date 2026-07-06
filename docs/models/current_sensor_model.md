# Current Sensor Model

This module emulates phase current sensing and ADC conversion.

## Inputs

- `Ia`
- `Ib`
- `Ic`

## Outputs

- `ADCA`
- `ADCB`
- `ADCC`

## Configuration

- `ADCResolution`
- `ADCReferenceVoltage`
- `Sensitivity`
- `OffsetA`
- `OffsetB`
- `OffsetC`
- `NoiseStdDev`
- `FilterAlpha`

## ADC Conversion

Each phase current is converted into a sensor voltage:

```text
Vsensor = Sensitivity * I + Offset + Noise
```

Then mapped to ADC counts:

```text
ADC = round((Vsensor / Vref) * (ADCResolution - 1))
```

The output is clamped to the valid ADC range.

## Noise

Noise is modeled as zero-mean Gaussian noise with standard deviation `NoiseStdDev`.

## Filtering

The output is optionally filtered with a first-order IIR filter:

```text
Y[k] = alpha * X[k] + (1 - alpha) * Y[k-1]
```

Where:

- `alpha = FilterAlpha`
- `0 <= alpha <= 1`

## Validation

- currents must be finite
- `ADCResolution > 0`
- `ADCReferenceVoltage > 0`
- `Sensitivity > 0`
- `NoiseStdDev >= 0`
- `FilterAlpha` must be in `[0, 1]`

