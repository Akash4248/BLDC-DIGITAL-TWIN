# Implementation Plan: Closing the Feedback Loop

To make this a true, professional Hardware-in-the-Loop system as outlined in your architecture, we need to implement the physical feedback wires. 

The ESP32 will now simulate the analog currents and digital encoder pulses, and the Arduino will physically read them!

## Open Questions

> [!WARNING]  
> - **RC Filters:** The ESP32 will generate high-frequency PWM for `Ia, Ib, Ic`. You MUST build three simple RC Low-Pass Filters (e.g., a 1kΩ resistor and a 0.1µF capacitor) to smooth these into steady analog voltages before feeding them into the Arduino's A0, A1, A2. Do you have these components?
> - **Arduino Pin Conflict:** Your Arduino currently uses **Pin 3** for the `AH` PWM pulse. The UNO only has hardware interrupts on Pins 2 and 3. Since Pin 3 is taken, I will use **Pin 2 (Hardware Interrupt)** for Encoder A, and **Pin 4 (Pin Change Interrupt)** for Encoder B. Is Pin 4 available?

## Proposed Changes

### 1. ESP32 Side (`firmware/firmware.ino` & `DigitalTwin.cpp`)
- **Simulation Speed:** I will increase the ESP32 simulation speed to 10 kHz (100 µs loop) to match your architecture document.
- **Analog Currents (`Ia, Ib, Ic`):** I will configure ESP32 `ledc` to output 39 kHz PWM on GPIO 4, 18, and 19. The PWM duty cycle will represent the simulated phase currents (0V = -Max Amps, 1.65V = 0 Amps, 3.3V = +Max Amps).
- **Encoder Outputs (`A, B, Z`):** I will map the ESP32's simulated Encoder state to GPIO 21, 22, and 23. To ensure the 10 kHz simulation loop can accurately generate the pulses without aliasing, I will set the Encoder resolution to 36 PPR (Pulses Per Revolution).

### 2. Arduino Side (New `FOC_Controller.ino`)
- I will create a brand new `FOC_Controller.ino` sketch for you in the workspace.
- **Encoder Interrupts:** It will replace the old "Analog A3 Angle" code with true Quadrature Encoder interrupt logic on Pins 2 and 4.
- **ADC Scaling:** It will read A0, A1, and A2, and mathematically map the 0-3.3V feedback signals back into true Amperage values for the PI Controllers.

## Verification Plan
1. I will write the code and provide you with the new wiring pinout.
2. You will flash the ESP32 and Arduino, and build the 3 RC filters.
3. You will verify that the Arduino accurately tracks the ESP32's simulated position and current!
