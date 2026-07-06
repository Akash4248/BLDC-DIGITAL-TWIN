# Twin Engine

This module combines the simulation subsystems into one deterministic fixed-step loop.

## Execution Order

```text
Receive USB
  -> Inverter
  -> Electrical Model
  -> Mechanical Model
  -> Hall
  -> Encoder
  -> Current Sensor
  -> Send Feedback
```

## Sample Rate

The engine runs at a fixed rate of `1000 Hz`.

```text
dt = 1 / 1000 = 0.001 s
```

## Determinism Rules

- no wall-clock timing in the solver
- fixed `dt` per step
- seeded sensor noise
- every subsystem is stepped exactly once per tick

## Torque Estimation

The engine estimates electromagnetic torque from phase currents using a sinusoidal PMSM approximation:

```text
i_q = (2/3) * [ia*sin(theta_e) + ib*sin(theta_e - 2*pi/3) + ic*sin(theta_e + 2*pi/3)]
T_e = Kt * i_q
```

## Loop Inputs

- USB telemetry packet containing:
  - `DutyA`
  - `DutyB`
  - `DutyC`
  - `Vdc`
  - `Timestamp`

## Loop Outputs

- inverter voltages
- electrical currents
- mechanical speed and angle
- Hall outputs
- encoder outputs
- current sensor ADC outputs
- USB feedback packet

