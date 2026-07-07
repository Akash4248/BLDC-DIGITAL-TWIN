#pragma once

#include <Arduino.h>

namespace TwinSimulation {

struct ElectricalConfig {
  float Rs;           // Stator resistance (Ohms)
  float Ls;           // Stator inductance (Henries) (Assuming Ld = Lq = Ls for simplicity)
  float Ke;           // Back-EMF constant (V / (rad/s))
  float polePairs;    // Number of pole pairs
};

struct ElectricalState {
  float ia;           // Phase A current (Amps)
  float ib;           // Phase B current (Amps)
  float ic;           // Phase C current (Amps)
  
  float ea;           // Phase A back-EMF (Volts)
  float eb;           // Phase B back-EMF (Volts)
  float ec;           // Phase C back-EMF (Volts)

  float van;          // Phase A to neutral voltage (Volts)
  float vbn;          // Phase B to neutral voltage (Volts)
  float vcn;          // Phase C to neutral voltage (Volts)
};

class ElectricalModel {
 public:
  ElectricalModel();

  // Initialize the model with motor parameters
  void begin(const ElectricalConfig& config);

  // Update the electrical simulation
  // va, vb, vc: Phase voltages from the inverter (Volts)
  // rotorSpeedMech: Mechanical rotor speed (rad/s)
  // rotorAngleMech: Mechanical rotor angle (rad)
  // dt: Time step for Euler integration (seconds)
  void update(float va, float vb, float vc, float rotorSpeedMech, float rotorAngleMech, float dt);

  // Read the calculated state
  const ElectricalState& getState() const;
  const ElectricalConfig& getConfig() const;

  // Set new parameters at runtime
  void setConfig(const ElectricalConfig& config);

 private:
  ElectricalState state_;
  ElectricalConfig config_;

  // Sine lookup or standard math can be used here.
  // Standard float math (sinf) will be used; we rely on Step 15 (CPU opt) to convert to fixed-point or LUTs later if needed.
};

} // namespace TwinSimulation
