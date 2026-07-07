#pragma once
// ============================================================
// MotorModel.h — Layer 4: Complete PMSM electrical + mechanical model
// Simulates a surface-mounted PMSM using Forward Euler integration.
// ============================================================
#include "Config.h"
#include "MotorState.h"

class MotorModel {
 public:
  MotorModel();

  // Advance simulation by one time step.
  // va, vb, vc: phase-to-neutral voltages from inverter model (V)
  // dt: timestep (s)
  void step(MotorState& state, float dt);

  // Reset internal integration state to zero
  void reset();

  // Set an external load torque (N·m). Default = 0.
  void setLoadTorque(float tl) { loadTorque_ = tl; }

 private:
  // State variables for Euler integration
  float ia_, ib_, ic_;    // Phase currents (A)
  float omega_;           // Mechanical speed (rad/s)
  float thetaMech_;       // Mechanical angle (rad)

  float loadTorque_;

  static float wrapAngle(float a);
};
