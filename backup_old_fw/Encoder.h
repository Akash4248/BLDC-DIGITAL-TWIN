#pragma once
// ============================================================
// Encoder.h - Gate signal reader (replaces physical encoder)
// Reads 6 PWM gate signals from Arduino using hardware interrupts.
// Computes duty cycles using cycle timing via xthal_get_ccount().
// ============================================================
#include <Arduino.h>
#include "Config.h"

class GateSignalReader {
 public:
  GateSignalReader() = default;

  // Configure all 6 interrupt pins and attach ISRs
  void begin();

  // Read the latest computed duty cycles (0.0 to 1.0)
  float getDutyA() const;
  float getDutyB() const;
  float getDutyC() const;
};
