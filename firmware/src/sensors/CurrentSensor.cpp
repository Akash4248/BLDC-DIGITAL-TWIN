#include "CurrentSensor.h"

namespace TwinSimulation {

CurrentSensor::CurrentSensor() {
  memset(&state_, 0, sizeof(state_));
  
  // Default configuration typical for a 3.3V system with 10-bit ADC (Arduino Uno)
  config_.gain = 0.1f;         // 100mV/A
  config_.offset = 2.5f;       // 2.5V center offset (for 5V system)
  config_.noiseStdDev = 0.05f; // 50mA noise
  config_.filterAlpha = 1.0f;  // No filtering by default
  config_.adcBits = 10;        // 10-bit ADC (0-1023)
  config_.adcReference = 5.0f; // 5.0V ADC reference
  
  adcMaxCounts_ = (1UL << config_.adcBits) - 1;
  filteredVoltage_ = config_.offset;
}

void CurrentSensor::begin(const CurrentSensorConfig& config) {
  setConfig(config);
  filteredVoltage_ = config_.offset;
  memset(&state_, 0, sizeof(state_));
}

void CurrentSensor::setConfig(const CurrentSensorConfig& config) {
  config_ = config;
  if (config_.adcBits < 1) config_.adcBits = 1;
  if (config_.adcBits > 16) config_.adcBits = 16;
  adcMaxCounts_ = (1UL << config_.adcBits) - 1;
}

float CurrentSensor::generateGaussianNoise(float stdDev) {
  if (stdDev <= 0.0f) return 0.0f;
  
  // Irwin-Hall approximation of Gaussian distribution (sum of 3 uniforms)
  // Uniform range: [-1, 1]. Variance of one uniform is 1/3. 
  // Sum of 3 has variance 1, so stddev is 1.
  float sum = 0.0f;
  for (int i = 0; i < 3; ++i) {
    // rand() returns 0 to RAND_MAX
    float u = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    sum += u;
  }
  
  return sum * stdDev;
}

void CurrentSensor::update(float trueCurrent) {
  state_.trueCurrent = trueCurrent;
  
  // 1. Add physical sensor noise
  state_.noisyCurrent = trueCurrent + generateGaussianNoise(config_.noiseStdDev);
  
  // 2. Convert current to voltage through analog frontend (Gain & Offset)
  float rawVoltage = (state_.noisyCurrent * config_.gain) + config_.offset;
  
  // 3. Apply RC Low-Pass Filter
  filteredVoltage_ += config_.filterAlpha * (rawVoltage - filteredVoltage_);
  
  // Clamp voltage to rails [0, adcReference]
  if (filteredVoltage_ < 0.0f) filteredVoltage_ = 0.0f;
  if (filteredVoltage_ > config_.adcReference) filteredVoltage_ = config_.adcReference;
  
  state_.analogVoltage = filteredVoltage_;
  
  // 4. ADC Quantization
  float adcFraction = filteredVoltage_ / config_.adcReference;
  float countsFloat = adcFraction * (float)adcMaxCounts_;
  
  // Convert to integer with rounding
  state_.adcCounts = static_cast<uint16_t>(countsFloat + 0.5f);
  
  if (state_.adcCounts > adcMaxCounts_) {
    state_.adcCounts = adcMaxCounts_;
  }
}

const CurrentSensorState& CurrentSensor::getState() const {
  return state_;
}

} // namespace TwinSimulation
