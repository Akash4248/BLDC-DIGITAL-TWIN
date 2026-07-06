# Mechanical Model

The mechanical model solves rotor speed and angle using Euler integration.

## Inputs

- `Torque`
- `LoadTorque` optional override

## State

- `Speed`
- `MechanicalAngle`

## Outputs

- `Speed`
- `MechanicalAngle`
- `ElectricalAngle`

## Equations

Rotor speed dynamics:

```text
dω/dt = (T_e - T_L - B * ω) / J
```

Where:

- `ω` is rotor speed in rad/s
- `T_e` is electromagnetic torque
- `T_L` is load torque
- `B` is viscous friction coefficient
- `J` is rotor inertia

Mechanical angle dynamics:

```text
dθ_m/dt = ω
```

Electrical angle:

```text
θ_e = PolePairs * θ_m
```

Euler integration:

```text
ω[k+1] = ω[k] + dt * dω/dt
θ_m[k+1] = θ_m[k] + dt * dθ_m/dt
```

## Modularity

The model is modular through a load-torque strategy interface:

- default behavior uses the load torque stored in motor parameters
- a custom torque-load model can inject time-varying disturbances

## Validation

- `Torque` must be finite
- `LoadTorque` override must be finite when provided
- `RotorInertia` must be greater than `0`
- `dt` must be greater than `0`

