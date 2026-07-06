# BLDC / PMSM Digital Twin Platform

Industrial digital twin stack featuring a C++ bare-metal Digital Twin engine running on an Arduino Uno, a Python telemetry backend, and a React dashboard.

## Architecture

```text
Arduino Uno (Simulation Core, 5V)
  -> 1 kHz Bare-Metal Twin Engine (Inverter, Physics, Sensors, Faults)
  -> UART binary protocol (TwinLink)
ESP32 (Telemetry Bridge, 3.3V)
  -> USB / Serial Bridge
Python FastAPI backend
  -> WebSocket telemetry broadcast
React dashboard
  -> Real-time charts, gauges, 3D visualization, and diagnostics
```

The Arduino Uno performs all physics calculations in a strict 1kHz `micros()`-timed loop, tracking CPU diagnostics to ensure it never misses a deadline. The Python backend acts as a high-speed router, bypassing its internal models to forward physical UART telemetry straight to the browser.

## Folder Structure

```text
backend/
  app/            FastAPI app, simulation service, twin engine, protocol, USB bridge
  tests/          Unit tests for protocol, models, broadcaster, and services
frontend/
  src/            React dashboard, charts, gauges, 3D scene, logger, parameter editor
firmware/
  firmware.ino    Main Arduino Uno sketch (1kHz timing loop)
  src/config.h    User-facing motor configuration
  src/main/       DigitalTwin engine and orchestration
  src/electrical/ Electrical and Inverter models
  src/mechanical/ Newton-Euler mechanical model
  src/simulation/ Torque and Thermal models
  src/sensors/    Encoder, Hall, and ADC current sensors
  src/faults/     Fault injection interceptors
  src/communication/ TwinLink binary protocol (CRC16)
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

When `FOC_USB_SERIAL_PORT` is set, the backend starts the USB bridge automatically and receives:

- Arduino -> ESP32 -> Backend: `Ia`, `Ib`, `Ic`, `RotorAngle`, `RotorSpeed`, `maxExecTime`, `missedDeadlines`

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

### Hardware-in-the-loop

- Flash `firmware.ino` to an Arduino Uno. Adjust `firmware/src/config.h` to match your motor parameters.
- Verify the serial port matches `FOC_USB_SERIAL_PORT`
- Check that the dashboard logger updates and the gauges visualize the live `TwinStatePacket` data.
- Watch the **Diagnostics** panel in the UI to ensure `maxExecTime` stays under 1000 microseconds.

## Notes

- The Arduino Uno twin engine is highly optimized, utilizing PROGMEM sine lookup tables (`FastMath.h`) and bitfield structs to fit within 2KB of SRAM.
- The C++ simulation loop is strictly deterministic at 1 kHz and uses a hardware Watchdog timer for safety.
