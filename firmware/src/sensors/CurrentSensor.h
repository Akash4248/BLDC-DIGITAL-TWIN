#pragma once

#include <Arduino.h>

namespace TwinSimulation {

struct CurrentSensorConfig {
  float gain;           // Volts per Amp (V/A)
  float offset;         // Zero-current offset voltage (V)
  float noiseStdDev;    // Standard deviation of Gaussian noise (Amps)
  float filterAlpha;    // First-order low-pass filter coefficient (0.0 to 1.0)
  uint16_t adcBits;     // ADC resolution (e.g. 10 for Uno, 12 for ESP32)
  float adcReference;   // ADC full-scale reference voltage (V)
};

struct CurrentSensorState {
  float trueCurrent;    // True input current
  float noisyCurrent;   // Current after noise
  float analogVoltage;  // Voltage after gain/offset and filtering
  uint16_t adcCounts;   // Final quantized digital value
};

class CurrentSensor {
 public:
  CurrentSensor();

  // Initialize with configuration
  void begin(const CurrentSensorConfig& config);

  // Update sensor state based on true current
  void update(float trueCurrent);

  // Read the calculated states
  const CurrentSensorState& getState() const;
  
  // Set configuration at runtime
  void setConfig(const CurrentSensorConfig& config);

 private:
  CurrentSensorState state_;
  CurrentSensorConfig config_;
  
  float filteredVoltage_; // State variable for the LPF
  uint32_t adcMaxCounts_; // Precalculated max ADC counts (e.g. 1023 for 10-bit)

  // Fast pseudo-random number generator for Gaussian noise approximation
  float generateGaussianNoise(float stdDev);
};

} // namespace TwinSimulation
