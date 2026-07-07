#pragma once
// ============================================================
// Inverter.h - 3-phase full-bridge inverter model
// Decodes 6 gate signals (AH,AL,BH,BL,CH,CL) into phase voltages.
// ============================================================

#include "Config.h"

struct InverterState {
  float va, vb, vc;  // Phase voltages (V)
};

class Inverter {
 public:
  Inverter() = default;

  // Decode gate signals and compute phase voltages.
  // dutyA/B/C: high-side duty cycle (0.0 to 1.0) derived from gate signals.
  // vdc: DC bus voltage (V)
  void update(float dutyA, float dutyB, float dutyC, float vdc);

  const InverterState& getState() const { return state_; }

 private:
  InverterState state_{};
};
