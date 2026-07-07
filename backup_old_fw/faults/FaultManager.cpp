#include "FaultManager.h"
#include "../utils/FastMath.h"

namespace TwinSimulation {

FaultManager::FaultManager() {
  clearAll();
}

void FaultManager::clearAll() {
  memset(&config_, 0, sizeof(config_));
}

void FaultManager::setConfig(const FaultConfig& config) {
  config_ = config;
}

const FaultConfig& FaultManager::getConfig() const {
  return config_;
}

float FaultManager::applyVoltageSag(float vdc) const {
  if (config_.voltageSag) {
    return vdc * 0.6f; // Emulate a 40% voltage sag
  }
  return vdc;
}

float FaultManager::applyOpenPhaseCurrent(float current, bool isOpen) const {
  if (isOpen) {
    return 0.0f; // Open phase cannot conduct current
  }
  return current;
}

float FaultManager::applyCurrentSensorFail(float current, bool isFailed) const {
  if (isFailed) {
    // Simulate a sensor railing high (e.g. op-amp output stuck to VCC)
    return 1000.0f; // Extremely high value that will saturate the ADC logic
  }
  return current;
}

bool FaultManager::applyHallFail(bool originalState, bool isFailed) const {
  if (isFailed) {
    return false; // Emulate a pulled-down broken wire
  }
  return originalState;
}

bool FaultManager::applyEncoderFail(bool originalState) const {
  if (config_.encoderFail) {
    return false; // Emulate broken encoder wire
  }
  return originalState;
}

float FaultManager::calculateImbalanceTorque(float mechanicalAngle, float baseInertia) const {
  if (config_.rotorImbalance) {
    // Imbalance creates a gravity/centrifugal sinusoidal load torque dependent on position
    return 0.5f * baseInertia * FastMath::sin(mechanicalAngle); 
  }
  return 0.0f;
}

float FaultManager::getExtraBearingFriction() const {
  if (config_.bearingFriction) {
    return 0.1f; // Add a flat 0.1 Nm of coulomb friction
  }
  return 0.0f;
}

float FaultManager::getHighLoadTorque() const {
  if (config_.highLoad) {
    return 2.5f; // Add 2.5 Nm of extra base load torque
  }
  return 0.0f;
}

} // namespace TwinSimulation
