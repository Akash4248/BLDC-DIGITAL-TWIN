#pragma once

#include <Arduino.h>

#include "../communication/TwinLink.h"
#include "../electrical/Inverter.h"
#include "../electrical/ElectricalModel.h"
#include "../simulation/TorqueModel.h"
#include "../mechanical/MechanicalModel.h"
#include "../sensors/HallSensor.h"
#include "../sensors/Encoder.h"
#include "../sensors/CurrentSensor.h"
#include "../simulation/ThermalModel.h"
#include "../faults/FaultManager.h"

namespace TwinSimulation {

class DigitalTwin {
 public:
  DigitalTwin(TwinProtocol::TwinLink& link);

  // Initialize all subsystems with default motor parameters
  void begin();

  // The main simulation step (must be called deterministically, e.g. at 1 kHz)
  // dt: integration time step in seconds (e.g. 0.001f for 1kHz)
  void step(float dt);
  
  // Access subsystems for configuration
  FaultManager& getFaults() { return faults_; }

  // Inject timing diagnostics to be sent in the next telemetry packet
  void setDiagnostics(uint16_t maxExecTime, uint16_t missedDeadlines);

  // Inject real physical duty cycles from HIL bridge
  void setExternalDutyCycles(float dutyA, float dutyB, float dutyC);

 private:
  TwinProtocol::TwinLink& link_;
  
  Inverter inverter_;
  ElectricalModel electrical_;
  TorqueModel torque_;
  MechanicalModel mechanical_;
  
  HallSensor hall_;
  Encoder encoder_;
  
  CurrentSensor currentA_;
  CurrentSensor currentB_;
  CurrentSensor currentC_;
  
  ThermalModel thermal_;
  FaultManager faults_;
  
  uint16_t diagMaxExecTime_;
  uint16_t diagMissedDeadlines_;
  
  bool useExtDuty_ = false;
  float extDutyA_ = 0.0f;
  float extDutyB_ = 0.0f;
  float extDutyC_ = 0.0f;
  
  void applyDefaultConfig();
};

} // namespace TwinSimulation
