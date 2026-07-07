#include "HallSensor.h"
#include <math.h>

namespace TwinSimulation {

constexpr float PI_F = 3.14159265359f;
constexpr float TWO_PI_F = 6.28318530718f;

HallSensor::HallSensor() {
  state_.a = false;
  state_.b = false;
  state_.c = false;
  
  // Default to 120 degree spacing
  config_.placementOffset = 0.0f;
  config_.sensorSpacing = TWO_PI_F / 3.0f;
}

void HallSensor::begin(const HallConfig& config) {
  config_ = config;
}

void HallSensor::setConfig(const HallConfig& config) {
  config_ = config;
}

bool HallSensor::isSensorHigh(float sensorAngle, float currentAngle) const {
  // Calculate relative angle difference
  float diff = currentAngle - sensorAngle;
  
  // Wrap to [0, 2PI)
  while (diff >= TWO_PI_F) diff -= TWO_PI_F;
  while (diff < 0.0f) diff += TWO_PI_F;
  
  // Hall sensor is HIGH for 180 electrical degrees (PI radians)
  return diff < PI_F;
}

void HallSensor::update(float electricalAngle) {
  // Angle of each sensor based on placement and spacing
  float angleA = config_.placementOffset;
  float angleB = config_.placementOffset + config_.sensorSpacing;
  float angleC = config_.placementOffset + (2.0f * config_.sensorSpacing);

  state_.a = isSensorHigh(angleA, electricalAngle);
  state_.b = isSensorHigh(angleB, electricalAngle);
  state_.c = isSensorHigh(angleC, electricalAngle);
}

const HallState& HallSensor::getState() const {
  return state_;
}

} // namespace TwinSimulation
