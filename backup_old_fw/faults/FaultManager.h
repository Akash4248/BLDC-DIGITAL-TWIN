#pragma once

#include <Arduino.h>

namespace TwinSimulation {

struct FaultConfig {
  uint16_t openPhaseA : 1;
  uint16_t openPhaseB : 1;
  uint16_t openPhaseC : 1;
  
  uint16_t lockedRotor : 1;
  uint16_t highLoad : 1;
  
  uint16_t currentSensorAFail : 1;
  uint16_t currentSensorBFail : 1;
  uint16_t currentSensorCFail : 1;
  
  uint16_t hallAFail : 1;
  uint16_t hallBFail : 1;
  uint16_t hallCFail : 1;
  
  uint16_t encoderFail : 1;
  
  uint16_t rotorImbalance : 1;
  uint16_t bearingFriction : 1;
  
  uint16_t voltageSag : 1;
  uint16_t overTemperatureTrigger : 1;
};

class FaultManager {
 public:
  FaultManager();

  // Update configuration (enable/disable specific faults)
  void setConfig(const FaultConfig& config);
  const FaultConfig& getConfig() const;

  // Clear all faults
  void clearAll();

  // Helper functions for the Main Loop to apply faults to signals
  
  // Electrical Faults
  float applyVoltageSag(float vdc) const;
  float applyOpenPhaseCurrent(float current, bool isOpen) const;
  
  // Sensor Faults
  float applyCurrentSensorFail(float current, bool isFailed) const;
  bool applyHallFail(bool originalState, bool isFailed) const;
  bool applyEncoderFail(bool originalState) const; // Can randomly toggle or force low
  
  // Mechanical Faults
  float calculateImbalanceTorque(float mechanicalAngle, float baseInertia) const;
  float getExtraBearingFriction() const;
  float getHighLoadTorque() const;

 private:
  FaultConfig config_;
};

} // namespace TwinSimulation
