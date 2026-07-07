#pragma once
// ============================================================
// MotorModel.h - Complete PMSM/BLDC Electrical + Mechanical Model
// Solves motor differential equations at 10 kHz using Forward Euler.
// ============================================================

#include "Config.h"

struct MotorState {
  // Electrical
  float ia, ib, ic;         // Phase currents (A)
  float torqueElec;          // Electromagnetic torque (N.m)

  // Mechanical
  float rotorSpeedRad;      // Mechanical rotor speed (rad/s)
  float rotorSpeedRPM;      // Mechanical rotor speed (RPM)
  float mechAngle;          // Mechanical rotor angle (rad)
  float elecAngle;          // Electrical rotor angle (rad), = mechAngle * pole_pairs
};

class MotorModel {
 public:
  MotorModel();

  // Step the motor simulation forward by one time step.
  // va, vb, vc: applied phase voltages (V) from inverter
  // dt: time step (s)
  void step(float va, float vb, float vc, float dt);

  // Apply an external mechanical load torque (N.m)
  void setLoadTorque(float tl) { loadTorque_ = tl; }

  const MotorState& getState() const { return state_; }

 private:
  MotorState state_{};
  float loadTorque_;

  // Internal phase currents (state variables for Euler integration)
  float ia_, ib_, ic_;       // Phase currents (A)
  float mechSpeed_;          // Mechanical speed (rad/s)
  float mechAngle_;          // Mechanical angle (rad)

  // Helper: wrap angle to 0..2pi
  float wrapAngle(float angle);
};
