# Backend Architecture

The backend is split into two layers:

## Simulation Layer

Owned by `backend/app/services/simulation_service.py` and `backend/app/twin/engine.py`.

This layer:

- owns the twin engine
- applies parameter updates
- handles simulation control
- injects faults
- records events in a bounded log buffer

## Optional USB Bridge

Owned by `backend/app/services/usb_bridge.py` and `backend/app/serial/connection.py`.

This layer:

- opens the ESP32 serial port with reconnect logic
- reads binary telemetry packets
- steps the twin engine at the incoming telemetry rate
- writes binary feedback packets back to the ESP32
- forwards each step to the WebSocket broadcaster

## Networking Layer

Owned by `backend/app/api/` and `backend/app/main.py`.

This layer:

- exposes REST endpoints
- exposes the WebSocket endpoint
- parses request payloads
- serializes responses

## Endpoint Summary

- `GET /api/v1/health`
- `GET /api/v1/simulation/state`
- `POST /api/v1/simulation/control/start`
- `POST /api/v1/simulation/control/stop`
- `POST /api/v1/simulation/control/reset`
- `PUT /api/v1/parameters`
- `POST /api/v1/faults/inject`
- `DELETE /api/v1/faults`
- `GET /api/v1/logs`
- `POST /api/v1/simulation/step`
- `WS /api/v1/ws/simulation`
- `WS /api/v1/ws/telemetry`

## Separation Rule

The routers do not implement simulation logic directly. They only call the simulation service.
