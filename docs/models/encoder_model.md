# Encoder Model

This module simulates an incremental encoder with quadrature A/B and an index pulse.

## Input

- `MechanicalAngle`

## Outputs

- `EncoderA`
- `EncoderB`
- `IndexPulse`

## Configuration

- `PPR`: pulses per revolution on channel A
- `ElectricalOffset`: mechanical angle phase offset
- `IndexWidthRad`: width of the index pulse window in radians
- `TransitionTime`: optional transition smoothing time

## Quadrature Equations

The encoder angle is wrapped to one revolution:

```text
theta = mod(MechanicalAngle, 2*pi)
```

Quadrature channels are generated from phase-shifted square waves:

```text
EncoderA = 1 if sin(PPR * theta + ElectricalOffset) >= 0 else 0
EncoderB = 1 if sin(PPR * theta + ElectricalOffset + pi/2) >= 0 else 0
```

## Index Pulse

The index pulse is asserted once per revolution inside a narrow angular window:

```text
IndexPulse = 1 if theta <= IndexWidthRad else 0
```

## Transition Animation

If `TransitionTime > 0`, outputs move toward their target values using Euler integration:

```text
dS/dt = (S_target - S) / TransitionTime
S[k+1] = S[k] + dt * dS/dt
```

Outputs are clamped to `[0, 1]`.

## Validation

- `PPR` must be a positive integer
- all angle/time values must be finite
- `IndexWidthRad` and `TransitionTime` must be greater than or equal to `0`

