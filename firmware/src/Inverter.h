#pragma once
// ============================================================
// Inverter.h — Layer 2 & 3: Gate signal reader + inverter model
// Reads 6 gate signals (AH, AL, BH, BL, CH, CL) and computes Va, Vb, Vc.
// ============================================================
#include <Arduino.h>
#include "Config.h"
#include "MotorState.h"

class Inverter {
 public:
  // Configure 6 gate input pins
  void begin();

  // Layer 2: Read current gate signal states from GPIO
  void readGates(MotorState& state);

  // Layer 3: Decode gate states into phase voltages
  void decode(MotorState& state, float vdc);
};
