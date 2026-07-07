#pragma once
// ============================================================
// CurrentOutput.h — Layer 5: Send Ia, Ib, Ic, Theta to Arduino
//
// GPIO25 (DAC1) → Ia  → Arduino A0  (true DAC, no RC needed)
// GPIO26 (DAC2) → Ib  → Arduino A1  (true DAC, no RC needed)
// GPIO4  (LEDC) → Ic  → RC filter → Arduino A2
// GPIO5  (LEDC) → Theta → RC filter → Arduino A3
//
// Voltage mapping (0..3.3V on a 5V Arduino = ADC 0..675):
//   Currents: -CURRENT_PEAK_A → 0V (DAC 0),  0A → 1.65V (DAC 127),  +PEAK → 3.3V (DAC 255)
//   Theta:    0 rad → 0V (DAC 0),  2π → 3.3V (DAC 255)
// ============================================================
#include <Arduino.h>
#include "Config.h"
#include "MotorState.h"

class CurrentOutput {
 public:
  // Configure DAC and LEDC PWM channels
  void begin();

  // Write phase currents and electrical angle to hardware outputs
  void write(const MotorState& state);
};
