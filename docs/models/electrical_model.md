# Electrical Model

The electrical model solves phase currents in the `a`, `b`, and `c` stator phases using a lumped phase-domain RL model with back-EMF.

## Inputs

- `Va`
- `Vb`
- `Vc`
- `RotorAngle`
- `Speed`

## Outputs

- `Ia`
- `Ib`
- `Ic`

## State

The solver is stateful and stores the previous currents:

```text
Ia[k], Ib[k], Ic[k]
```

## Back-EMF Equations

The electrical angle is derived from the mechanical rotor angle:

```text
theta_e = PolePairs * RotorAngle
```

Phase back-EMF is modeled sinusoidally:

```text
e_a = Ke * Speed * sin(theta_e)
e_b = Ke * Speed * sin(theta_e - 2*pi/3)
e_c = Ke * Speed * sin(theta_e + 2*pi/3)
```

This module treats `Speed` as mechanical speed in rad/s.

## Current Dynamics

Each phase current is solved independently in the phase domain:

```text
dIa/dt = (Va - Rs * Ia - e_a) / Ls
dIb/dt = (Vb - Rs * Ib - e_b) / Ls
dIc/dt = (Vc - Rs * Ic - e_c) / Ls
```

Where the lumped phase inductance is:

```text
Ls = (Ld + Lq) / 2
```

## Euler Integration

The solver uses explicit Euler integration:

```text
Ia[k+1] = Ia[k] + dt * dIa/dt
Ib[k+1] = Ib[k] + dt * dIb/dt
Ic[k+1] = Ic[k] + dt * dIc/dt
```

This is intentionally modular:

- back-EMF generation is injected via a strategy interface
- integration is isolated in a dedicated Euler integrator
- the solver keeps internal current state between steps

## Validation

- All numeric inputs must be finite
- `dt` must be greater than `0`
- `Rs`, `Ld`, and `Lq` must define a positive inductance

