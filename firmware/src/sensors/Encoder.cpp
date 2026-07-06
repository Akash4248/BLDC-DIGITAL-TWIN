#include "Encoder.h"
#include <math.h>

namespace TwinSimulation {

constexpr float TWO_PI_F = 6.28318530718f;

Encoder::Encoder() {
  state_.a = false;
  state_.b = false;
  state_.index = false;
  
  config_.ppr = 1024;
  config_.indexOffset = 0.0f;
  indexPulseWidth_ = TWO_PI_F / (config_.ppr * 4.0f);
}

void Encoder::begin(const EncoderConfig& config) {
  setConfig(config);
}

void Encoder::setConfig(const EncoderConfig& config) {
  config_ = config;
  if (config_.ppr == 0) {
    config_.ppr = 1; // Prevent division by zero
  }
  // The index pulse width is typically 1 quadrature state (1/4 of a pulse)
  indexPulseWidth_ = TWO_PI_F / (config_.ppr * 4.0f);
}

void Encoder::update(float mechanicalAngle) {
  // 1. Calculate the quadrature phase (0.0 to 1.0) within a single pulse
  float pulses = (mechanicalAngle / TWO_PI_F) * config_.ppr;
  float phase = pulses - floorf(pulses); // Fractional part

  // 2. Determine A and B states based on quadrature timing
  state_.a = (phase < 0.5f);
  state_.b = (phase >= 0.25f && phase < 0.75f);

  // 3. Calculate Index (Z) pulse state
  float indexAngle = mechanicalAngle - config_.indexOffset;
  
  // Wrap to [0, 2*PI)
  while (indexAngle >= TWO_PI_F) indexAngle -= TWO_PI_F;
  while (indexAngle < 0.0f) indexAngle += TWO_PI_F;

  // Index is high only during a very narrow window (one quadrature step)
  state_.index = (indexAngle < indexPulseWidth_);
}

const EncoderState& Encoder::getState() const {
  return state_;
}

} // namespace TwinSimulation
