#pragma once
// ============================================================
// CurrentOutput.h - Analog current and theta output to Arduino
// Uses ESP32 LEDC (PWM) to synthesize analog voltages via RC filter.
// ============================================================
#include "Config.h"

class CurrentOutput {
 public:
  CurrentOutput() = default;

  // Configure LEDC PWM channels for all 4 outputs (Ia, Ib, Ic, Theta)
  void begin();

  // Write phase currents (-CURRENT_PEAK_A..+CURRENT_PEAK_A) -> 0..3.3V PWM
  void writeCurrent(float ia, float ib, float ic);

  // Write electrical angle (0..2pi) -> 0..3.3V PWM
  void writeTheta(float theta_rad);
};
