# Inverter Model

This module maps normalized duty cycles to phase voltages.

## Inputs

- `DutyA`
- `DutyB`
- `DutyC`
- `Vdc`

## Outputs

- `Va`
- `Vb`
- `Vc`

## Validation

- `DutyA`, `DutyB`, `DutyC` must be finite numbers in `[0, 1]`
- `Vdc` must be a finite number greater than `0`

## Sinusoidal PWM

For sinusoidal PWM, the phase voltage relative to the inverter midpoint is:

```text
Vx = (2 * DutyX - 1) * Vdc / 2
```

Where `X` is `A`, `B`, or `C`.

Expanded:

```text
Va = (2 * DutyA - 1) * Vdc / 2
Vb = (2 * DutyB - 1) * Vdc / 2
Vc = (2 * DutyC - 1) * Vdc / 2
```

## SVPWM

For SVPWM-style common-mode removal, first compute the duty average:

```text
d_avg = (DutyA + DutyB + DutyC) / 3
```

Then synthesize phase voltages as:

```text
Va = (DutyA - d_avg) * Vdc
Vb = (DutyB - d_avg) * Vdc
Vc = (DutyC - d_avg) * Vdc
```

This removes the common-mode component so the three phase voltages sum to zero:

```text
Va + Vb + Vc = 0
```

## Dead-Time Compensation Interface

Dead-time correction is applied before voltage synthesis through an external strategy:

```text
(DutyA', DutyB', DutyC') = compensate(DutyA, DutyB, DutyC, Vdc)
```

The inverter model does not prescribe the compensation method. That allows current-direction-aware, hardware-specific implementations without changing the voltage equations.

