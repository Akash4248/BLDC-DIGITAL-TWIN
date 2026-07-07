# Hardware-in-the-Loop Implementation Plan

You are setting up a highly professional **3-Board Hardware-in-the-Loop (HIL)** testbench!
1. **Arduino UNO #1 (FOC Controller):** Generates physical 5V PWM pulses.
2. **Arduino UNO #2 (Bridge):** Reads the 5V PWM pulses, calculates the duty cycles, and sends them via 5V UART TX to the ESP32 RX (which is 3.3V safe if you use a simple resistor divider).
3. **ESP32 (Digital Twin):** Calculates the motor physics based on the bridge data, and streams the results to the PC dashboard via USB.

## Open Questions

> [!IMPORTANT]
> - Are you using an **ESP32** or an **ESP8266**? (The plan assumes ESP32 since it has multiple hardware serial ports like `Serial2`, whereas the ESP8266 does not).
> - For Arduino UNO #2 to ESP32 communication, I plan to use a fast 115200 baud binary packet. You will need to wire Arduino #2 `TX` to ESP32 `RX2` (GPIO 16). Does this wiring work for you?

## Proposed Changes

### Arduino UNO #2 (Bridge Sketch)
I will write a brand new, standalone Arduino sketch (`bridge/PWM_to_Serial_Bridge.ino`) for your second UNO. 
- It will use a highly optimized register-polling loop (`PIND` and `PINB`) to sample the High-side PWM pins (3, 6, and 10) thousands of times per millisecond.
- This guarantees an extremely accurate duty-cycle measurement without crashing the Arduino or relying on complex interrupts.
- It will send a tiny 6-byte binary packet `[Header1, Header2, DutyA, DutyB, DutyC, Checksum]` to the ESP32 at 115200 baud.

### ESP32 (Digital Twin Firmware)

#### [MODIFY] `firmware.ino`
- Initialize `Serial2` (Hardware UART) to receive data from the Bridge.
- Parse the 6-byte binary packet from the Bridge to extract real-time `dutyA`, `dutyB`, and `dutyC`.
- Continue listening to `Serial` (USB to PC) to receive Load Torque and Fault parameters from the dashboard.
- Feed the merged data (Bridge Duty Cycles + PC Load Torque) into `twin.step()`.

#### [MODIFY] `src/main/DigitalTwin.cpp`
- Expose a method to allow `firmware.ino` to inject the externally read Duty Cycles, overriding the default behavior of reading them entirely from the PC telemetry packet.

## Verification Plan

### Automated/Code Verification
- I will verify that the binary packet parsing on the ESP32 includes checksum validation so noise on the RX line doesn't crash the physics simulation.

### Manual Verification
- You will flash Arduino #2 and open the Serial Plotter to verify it accurately reads the PWMs from Arduino #1.
- You will flash the ESP32, wire the TX/RX, and verify the Dashboard reacts to the physical FOC controller!
