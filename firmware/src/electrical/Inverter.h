#pragma once

#include <Arduino.h>

namespace TwinSimulation {

struct InverterState {
  float va;
  float vb;
  float vc;
  bool shootThroughFault;
  bool invalidState;
};

class Inverter {
 public:
  Inverter();

  // Initialize inverter parameters
  // deadTimeUs: The expected dead-time between high/low switches
  void begin(float deadTimeUs = 1.0f);

  // Update the inverter state based on raw gate signals
  // This calculates the effective phase voltages applied to the motor
  void update(bool ah, bool al, bool bh, bool bl, bool ch, bool cl, float vdc);

  // Read the calculated state
  const InverterState& getState() const;

  // Clear faults (e.g. after a reset)
  void clearFaults();

 private:
  InverterState state_;
  float deadTimeUs_;

  // Calculate phase voltage for a single half-bridge leg
  float calculateLegVoltage(bool highGate, bool lowGate, float vdc, bool& shootThrough);
};

} // namespace TwinSimulation
