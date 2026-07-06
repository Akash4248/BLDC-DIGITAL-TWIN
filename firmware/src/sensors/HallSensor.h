#pragma once

#include <Arduino.h>

namespace TwinSimulation {

struct HallConfig {
  float placementOffset; // Global angle offset (rad) for all sensors relative to electrical zero
  float sensorSpacing;   // Spacing between sensors (rad). Typically 120 degrees (2*PI/3)
};

struct HallState {
  uint8_t a : 1;
  uint8_t b : 1;
  uint8_t c : 1;
};

class HallSensor {
 public:
  HallSensor();

  // Initialize with configuration
  void begin(const HallConfig& config);

  // Update hall states based on current electrical angle
  // electricalAngle must be wrapped to [0, 2*PI]
  void update(float electricalAngle);

  // Read the calculated states
  const HallState& getState() const;
  
  // Set configuration at runtime (e.g. alignment procedure)
  void setConfig(const HallConfig& config);

 private:
  HallState state_;
  HallConfig config_;

  // Helper to determine if a sensor is HIGH based on its specific angle
  bool isSensorHigh(float sensorAngle, float currentAngle) const;
};

} // namespace TwinSimulation
