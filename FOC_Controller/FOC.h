// FOC.h - Top-level FOC algorithm orchestrator
#pragma once
#include "Config.h"
#include "PI.h"

class FOCController {
 public:
  FOCController();

  // Initialize the FOC controller (resets integrators, sets up PWM)
  void begin();

  // Execute one complete FOC cycle (call from 10 kHz ISR or loop)
  // dt: time step in seconds
  void step(float dt);

  // Set the target speed in RPM (called from Serial or external command)
  void setTargetRPM(float rpm);

  // Get the current measured speed in RPM
  float getMeasuredRPM() const;

  // Get measured currents (for debug/telemetry)
  float getId() const { return id_; }
  float getIq() const { return iq_; }
  float getTheta() const { return theta_; }

 private:
  // PI Controllers
  PIController pi_id_;
  PIController pi_iq_;
  PIController pi_speed_;

  // State
  float theta_;       // Electrical rotor angle (radians, 0..2pi)
  float id_, iq_;     // DQ currents
  float mechSpeedRPM_;
  float openLoopTheta_;  // Internal open-loop angle accumulator

  // Target
  float targetSpeedRad_; // Target speed in rad/s
};
