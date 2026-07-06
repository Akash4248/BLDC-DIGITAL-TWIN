#pragma once

#include <Arduino.h>
#include "../electrical/ElectricalModel.h"
#include "../mechanical/MechanicalModel.h"

namespace TwinSimulation {

struct TorqueState {
  float electromagneticTorque;  // Te (Nm)
  float electricalPower;        // Input electrical power (W)
  float mechanicalPower;        // Converted mechanical power (W)
  float efficiency;             // Efficiency (0.0 to 1.0)
};

class TorqueModel {
 public:
  TorqueModel();

  // Computes torque and power based on current electrical and mechanical states.
  // This must be called AFTER ElectricalModel and BEFORE MechanicalModel in the loop.
  void update(const ElectricalState& elecState, 
              const ElectricalConfig& elecConfig,
              float mechanicalSpeed,
              float electricalAngle);

  // Read the calculated state
  const TorqueState& getState() const;

 private:
  TorqueState state_;
};

} // namespace TwinSimulation
