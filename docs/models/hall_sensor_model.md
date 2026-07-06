# Hall Sensor Model

This module generates digital Hall sensor outputs from electrical angle.

## Input

- `ElectricalAngle`

## Outputs

- `HallA`
- `HallB`
- `HallC`

## Configurable Placement

Hall placement is controlled by a single electrical angle offset:

```text
ElectricalOffset
```

This offset lets the model align to the physical sensor placement on a motor or PCB.

## Sector Mapping

The electrical cycle is divided into six 60-degree sectors:

```text
sector = floor(mod(theta + offset, 2*pi) / (pi/3))
```

The canonical Hall patterns are:

```text
sector 0 -> 101
sector 1 -> 100
sector 2 -> 110
sector 3 -> 010
sector 4 -> 011
sector 5 -> 001
```

## Transition Animation

Hall outputs can be animated using a first-order transition model:

```text
dH/dt = (H_target - H) / TransitionTime
```

Integrated with Euler:

```text
H[k+1] = H[k] + dt * dH/dt
```

When `TransitionTime` is `0`, the outputs switch instantly.

When `TransitionTime` is greater than `0`, outputs smoothly move toward the target state and are clamped to `[0, 1]`.

## Validation

- `ElectricalAngle` must be finite
- offsets must be finite
- `ElectricalOffset` must be finite
- `TransitionTime` must be greater than or equal to `0`
