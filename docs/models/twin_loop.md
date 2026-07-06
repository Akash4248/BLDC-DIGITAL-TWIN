# Twin USB Loop

This wrapper connects the USB binary protocol to the deterministic twin engine.

## Pipeline

```text
USB frame -> decode telemetry -> step twin engine -> encode feedback frame -> USB frame
```

## Determinism

The loop is deterministic because it performs a single fixed-step engine update per input frame and does not use wall-clock timing.

## Error Handling

- malformed binary frames are rejected by the protocol decoder
- non-telemetry USB packets are rejected
- the engine can be reset to a known state for repeatable behavior

