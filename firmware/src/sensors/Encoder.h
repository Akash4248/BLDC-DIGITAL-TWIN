#pragma once

#include <Arduino.h>

namespace TwinSimulation {

struct EncoderConfig {
  uint32_t ppr;             // Pulses Per Revolution
  float indexOffset;        // Mechanical angle offset for the index pulse (rad)
};

struct EncoderState {
  uint8_t a : 1;
  uint8_t b : 1;
  uint8_t index : 1;
};

class Encoder {
 public:
  Encoder();

  // Initialize with configuration
  void begin(const EncoderConfig& config);

  // Update encoder states based on current mechanical angle
  // mechanicalAngle must be wrapped to [0, 2*PI)
  void update(float mechanicalAngle);

  // Read the calculated states
  const EncoderState& getState() const;
  
  // Set configuration at runtime
  void setConfig(const EncoderConfig& config);

 private:
  EncoderState state_;
  EncoderConfig config_;
  
  float indexPulseWidth_; // Pre-calculated width of the index pulse (rad)
};

} // namespace TwinSimulation
