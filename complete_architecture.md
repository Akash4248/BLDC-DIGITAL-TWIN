# Complete Project Architecture
**Hardware-in-the-Loop (HIL) Digital Twin FOC System**

```mermaid
graph TD
    %% Arduino Block
    subgraph Arduino_UNO ["Arduino UNO (FOC Controller)"]
        direction TB
        ADC["ADC (A0, A1, A2)"]
        INT["Hardware INT (D2, D4)"]
        Math["FOC Math (Clarke, Park, PI, SVPWM)"]
        PWM_OUT["PWM Timers (D3, D6, D10)"]
        
        ADC --> Math
        INT --> Math
        Math --> PWM_OUT
    end

    %% ESP32 Block
    subgraph ESP32 ["ESP32 (Motor Digital Twin)"]
        direction TB
        PWM_IN["Hardware INT & xthal_get_ccount() (GPIO 32, 33, 25)"]
        PhysEngine["Physics Engine (10 kHz)"]
        DAC["LEDC PWM Outputs (GPIO 4, 18, 19)"]
        ENC_OUT["Digital GPIO (21, 22)"]
        UART["UART Telemetry (921600 Baud)"]

        PWM_IN --> PhysEngine
        PhysEngine --> DAC
        PhysEngine --> ENC_OUT
        PhysEngine --> UART
    end

    %% Physical Connections
    PWM_OUT -- "3x High-Side Gate Pulses\n(Through Voltage Dividers)" --> PWM_IN
    DAC -- "3x PWM Phase Currents\n(Through RC Low-Pass Filters)" --> ADC
    ENC_OUT -- "2x Encoder Pulses (A, B)" --> INT

    %% PC Block
    subgraph PC_Dashboard ["PC (Web Dashboard)"]
        Backend["Python FastAPI Backend"]
        Frontend["React + Three.js UI"]
        
        Backend --> Frontend
    end
    
    UART -- "Downsampled 1kHz Binary Packets" --> Backend
```

---

## Detailed Data Flow Diagram

```mermaid
sequenceDiagram
    participant UNO as Arduino UNO (FOC)
    participant HW as Hardware Wires
    participant ESP as ESP32 (Digital Twin)
    participant UI as PC Dashboard
    
    Note over UNO, HW: [1] Forward Command Path
    UNO->>HW: Outputs PWM (D3, D6, D10)
    HW->>ESP: 3x Gate Pulses (5V to 3.3V via divider)
    
    Note over ESP: [2] High-Speed Simulation Loop (10 kHz)
    ESP->>ESP: Measure pulse width via Hardware Interrupts
    ESP->>ESP: Solve Electrical Equations (Ia, Ib, Ic)
    ESP->>ESP: Solve Mechanical Equations (Torque, Speed, Angle)
    
    Note over ESP, HW: [3] Closed-Loop Feedback Path
    ESP->>HW: Outputs ledc PWM (GPIO 4, 18, 19)
    HW->>UNO: Analog Currents (Ia, Ib, Ic via RC Filters)
    
    ESP->>HW: Outputs Digital A/B Pulses (GPIO 21, 22)
    HW->>UNO: Quadrature Encoder Interrupts
    
    Note over UNO: [4] FOC Math (1 kHz Loop)
    UNO->>UNO: Update Rotor Angle from Interrupts
    UNO->>UNO: Read Analog Phase Currents
    UNO->>UNO: Clarke -> Park -> PI Control -> Inverse Park -> SVPWM
    
    Note over ESP, UI: [5] Telemetry Path (1 kHz)
    ESP->>UI: Serial UART Packets (TwinProtocol)
    UI->>UI: Update 3D Motor, Plots, Vectors
```

---

## Software Architecture

### Arduino UNO (FOC Controller)
Runs a 1 kHz closed-loop execution timer inside the main loop.
1. **`isr_encA()` & `ISR(PCINT2_vect)`**: Track mechanical rotor position constantly in the background.
2. **`readInputs()`**: Samples A0, A1, A2, and scales the 0-3.3V ESP32 analog signal into `Amps`. Converts encoder ticks into electrical angle `theta`.
3. **`clarke()` & `park()`**: Translates 3-phase AC currents into DC D/Q currents.
4. **`currentPI()`**: Compares `Id` and `Iq` against target setpoints and computes the necessary `Vd` and `Vq` voltages to correct the error.
5. **`inversePark()`**: Translates D/Q voltages back into 2-phase alpha/beta stationary frame.
6. **`SVPWM()`**: Injects third-harmonic vectors and calculates exact duty cycles, loading them directly into `OCR0A`, `OCR1A`, and `OCR2B` hardware timer registers.

### ESP32 (Motor Emulator)
Runs a highly optimized 10 kHz (100 µs) physics loop on Core 1.
1. **Hardware Interrupts**: 3x `CHANGE` interrupts trigger on any incoming PWM edge. They use the ESP32 internal 240MHz cycle counter (`xthal_get_ccount()`) to calculate the exact duty cycle with 15-bit nanosecond precision.
2. **`Inverter.cpp`**: Takes the 3 duty cycles and multiplies by bus voltage to compute `Va`, `Vb`, `Vc`.
3. **`ElectricalModel.cpp`**: Solves differential equations to calculate `Ia`, `Ib`, `Ic` based on phase voltages, back EMF, resistance, and inductance.
4. **`TorqueModel.cpp`**: Computes the electromagnetic torque.
5. **`MechanicalModel.cpp`**: Solves inertia and friction equations to update rotor speed and angle.
6. **Output Stage**:
   - `Ia, Ib, Ic` are mapped to 8-bit values and written to 39 kHz hardware PWM channels (`ledcWrite`).
   - The rotor angle is passed to `Encoder.cpp`, which calculates A/B/Z states and writes them to digital GPIO pins.
7. **Telemetry**: Once every 10 loops (1 kHz), the ESP32 packages the internal state and blasts it over UART to the Python backend.

---

## Folder Structure

```text
Project_Root/
├── FOC_Controller/                 # Arduino UNO Code
│   └── FOC_Controller.ino          # Complete FOC logic + Encoder Interrupts
│
├── firmware/                       # ESP32 Digital Twin Code
│   ├── firmware.ino                # 10kHz Main Loop, Interrupts, LEDC Output
│   └── src/
│       ├── main/
│       │   ├── DigitalTwin.h       # Aggregates all physics models
│       │   └── DigitalTwin.cpp     # 10kHz step() pipeline
│       ├── electrical/
│       │   ├── ElectricalModel.cpp # V = I*R + L*(di/dt) + EMF
│       │   └── Inverter.cpp        # Duty Cycle -> Phase Voltage
│       ├── mechanical/
│       │   └── MechanicalModel.cpp # T = J*(dw/dt) + B*w
│       ├── simulation/
│       │   └── TorqueModel.cpp     
│       ├── sensors/
│       │   └── Encoder.cpp         # Rotor Angle -> A/B Pulses
│       ├── faults/
│       │   └── FaultManager.cpp    # Injects failures dynamically
│       └── communication/
│           └── TwinLink.cpp        # 921600 Baud Binary Protocol
│
├── backend/                        # Python FastApi server (Reads UART)
├── frontend/                       # React & Three.js (3D Visualization)
└── walkthrough.md                  # Wiring Instructions
```
