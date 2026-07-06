# BLDC / PMSM Digital Twin Platform

Industrial digital twin stack for an ESP32 FOC controller, a Python twin engine, and a React dashboard.

## Architecture

```text
ESP32
  -> USB binary protocol
  -> Python FastAPI backend
  -> 1 kHz Digital Twin engine
  -> WebSocket telemetry
  -> React dashboard
  -> React Three Fiber 3D visualization
```

The backend can also open a USB bridge to the ESP32 and exchange binary packets directly when the serial port is configured.

## Folder Structure

```text
backend/
  app/            FastAPI app, simulation service, twin engine, protocol, USB bridge
  tests/          Unit tests for protocol, models, broadcaster, and services
frontend/
  src/            React dashboard, charts, gauges, 3D scene, logger, parameter editor
firmware/
  src/comm/       ESP32 binary protocol implementation
docs/
  models/         Mathematical model documentation
  protocol/       Packet format and sequence diagrams
```

## Components

- `backend/app/serial/protocol.py`: binary packet framing, CRC16, validation
- `backend/app/services/usb_bridge.py`: optional ESP32 USB bridge
- `backend/app/twin/engine.py`: deterministic 1 kHz simulation loop
- `frontend/src/visualization/MotorScene.tsx`: React Three Fiber motor scene
- `frontend/src/charts/TelemetryCharts.tsx`: Plotly waveforms and CSV export
- `frontend/src/charts/Gauges.tsx`: live gauges
- `frontend/src/components/LoggerPanel.tsx`: backend event log viewer

## Build

### Backend

Create a virtual environment and install the Python dependencies:

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install fastapi uvicorn pyserial pytest
```

Run the backend:

```bash
python3 -m uvicorn backend.app.main:app --reload --host 0.0.0.0 --port 8000
```

Optional USB bridge to the ESP32:

```bash
export FOC_USB_SERIAL_PORT=/dev/ttyUSB0
export FOC_USB_BAUDRATE=921600
python3 -m uvicorn backend.app.main:app --reload --host 0.0.0.0 --port 8000
```

When `FOC_USB_SERIAL_PORT` is set, the backend starts the USB bridge automatically and exchanges:

- ESP32 -> backend: `DutyA`, `DutyB`, `DutyC`, `Vdc`, `Timestamp`
- backend -> ESP32: `Ia`, `Ib`, `Ic`, `RotorAngle`, `RotorSpeed`

### Frontend

Install dependencies and run Vite:

```bash
cd frontend
npm install
npm run dev
```

Production build:

```bash
cd frontend
npm run build
```

## Run

Default local endpoints:

- Backend HTTP: `http://localhost:8000`
- Backend WebSocket: `ws://localhost:8000/api/v1/ws/telemetry`
- Frontend: `http://localhost:5173`

The dashboard connects to the backend over WebSocket and polls the REST API for state, parameters, faults, and logs.

## Test

### Backend

```bash
python3 -m pytest backend/tests
```

### Frontend

```bash
cd frontend
npm run test
```

### Protocol / hardware-in-the-loop

- Confirm the ESP32 uses the binary framing in `firmware/src/comm/Protocol.cpp`
- Verify the serial port matches `FOC_USB_SERIAL_PORT`
- Check that the backend logs show `simulation.step` events and that the dashboard logger updates

## Notes

- The twin engine is deterministic at 1 kHz when driven by the telemetry stream.
- The backend simulation and networking layers are separated.
- The dashboard remains usable without hardware by connecting to the backend’s built-in simulation state.
