# BLDC / PMSM Digital Twin USB Binary Protocol

This document defines the binary frames exchanged between the ESP32 FOC controller and the Python digital twin backend.

## Goals

- Fixed, deterministic binary packets
- Strong validation with header checks and CRC16
- Stream-safe parsing over USB serial
- Independent implementation on ESP32 and Python
- Easy to extend without breaking existing frames

## Frame Layout

All frames use little-endian encoding.

```text
| Sync(2) | Ver(1) | Type(1) | Seq(2) | PayloadLen(2) | Reserved(2) | Payload(N) | CRC16(2) |
```

### Field Details

- `Sync`: constant `0xAA55`
- `Ver`: protocol version, currently `1`
- `Type`: packet type identifier
- `Seq`: monotonically increasing sequence number
- `PayloadLen`: number of bytes in `Payload`
- `Reserved`: currently `0x0000`
- `CRC16`: CRC16-CCITT-FALSE over all bytes from `Ver` through end of `Payload`

## Packet Types

### ESP32 -> Python: Motor Telemetry

Type: `0x01`

Payload:

```text
| TimestampUs(u64) | DutyA(f32) | DutyB(f32) | DutyC(f32) | Vdc(f32) |
```

Fields:

- `TimestampUs`: microsecond timestamp from the ESP32
- `DutyA`: phase A duty cycle, normalized `0.0..1.0`
- `DutyB`: phase B duty cycle, normalized `0.0..1.0`
- `DutyC`: phase C duty cycle, normalized `0.0..1.0`
- `Vdc`: measured DC bus voltage

Payload length: `24` bytes

### Python -> ESP32: Twin State

Type: `0x02`

Payload:

```text
| Ia(f32) | Ib(f32) | Ic(f32) | RotorAngle(f32) | RotorSpeed(f32) |
```

Fields:

- `Ia`, `Ib`, `Ic`: phase currents in amperes
- `RotorAngle`: electrical rotor angle in radians
- `RotorSpeed`: rotor speed in rad/s

Payload length: `20` bytes

## Validation Rules

A frame is valid only if all of the following are true:

1. Sync bytes equal `0xAA55`
2. Protocol version matches supported version
3. Packet type is known
4. `PayloadLen` matches the expected length for the packet type
5. CRC16 matches the frame contents

## Error Handling Rules

- Invalid sync: discard one byte and rescan
- Unknown version: reject frame
- Unknown packet type: reject frame
- Length mismatch: reject frame
- CRC mismatch: reject frame
- Partial frame: keep buffering until complete or timeout

## Extensibility

The `Type` field is reserved for future packet types such as:

- diagnostics
- calibration commands
- parameter updates
- acknowledgements

Backward-compatible additions should introduce new packet types, not modify existing payload layouts.

