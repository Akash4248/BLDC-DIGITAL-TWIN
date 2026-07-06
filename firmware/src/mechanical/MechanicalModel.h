#pragma once

#include <Arduino.h>

namespace TwinSimulation {

struct MechanicalConfig {
  float inertia;      // Rotor inertia (J) in kg*m^2
  float frictionB;    // Viscous friction coefficient (B) in Nm/(rad/s)
  float frictionC;    // Coulomb/static friction (Tf) in Nm
  float polePairs;    // Number of pole pairs
};

struct MechanicalState {
  float rotorSpeed;       // Mechanical speed (rad/s)
  float rotorAngle;       // Mechanical angle (rad) wrapped to [0, 2*PI]
  float electricalAngle;  // Electrical angle (rad) wrapped to [0, 2*PI]
  float acceleration;     // Mechanical acceleration (rad/s^2)
};

class MechanicalModel {
 public:
  MechanicalModel();

  // Initialize the model with mechanical parameters
  void begin(const MechanicalConfig& config);

  // Update the mechanical simulation
  // electromagneticTorque: Te applied by the motor (Nm)
  // loadTorque: Tl applied externally (Nm)
  // dt: Time step for Euler integration (seconds)
  void update(float electromagneticTorque, float loadTorque, float dt);

  // Read the calculated state
  const MechanicalState& getState() const;
  const MechanicalConfig& getConfig() const;

  // Set new parameters at runtime
  void setConfig(const MechanicalConfig& config);
  
  // Set load torque at runtime (e.g. from an external fault or profile)
  void setLoadTorque(float loadTorque);
  float getLoadTorque() const;

 private:
  MechanicalState state_;
  MechanicalConfig config_;
  float currentLoadTorque_;

  // Helper to wrap angles to [0, 2*PI)
  float wrapAngle(float angle);
};

} // namespace TwinSimulation
