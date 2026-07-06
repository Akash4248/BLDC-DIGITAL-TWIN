#pragma once

#include <Arduino.h>

namespace TwinSimulation {
namespace FastMath {

// Precomputed constants
constexpr float PI_F = 3.14159265359f;
constexpr float TWO_PI_F = 6.28318530718f;
constexpr float INV_TWO_PI = 1.0f / TWO_PI_F;

// Extremely fast PROGMEM-based sine approximation for AVR
// rad: angle in radians (does not need to be wrapped perfectly, but wrapping is safer)
// Returns: float between -1.0 and 1.0
float sin(float rad);

// Fast angle wrapping to [0, 2PI)
// Optimized for small positive increments (like integrating speed)
inline float wrapAngle(float angle) {
  if (angle >= TWO_PI_F) {
    angle -= TWO_PI_F;
    if (angle >= TWO_PI_F) {
      // If it's way out of bounds, fallback to slower wrapping
      angle -= (int)(angle * INV_TWO_PI) * TWO_PI_F;
    }
  } else if (angle < 0.0f) {
    angle += TWO_PI_F;
    if (angle < 0.0f) {
      angle -= (int)((angle * INV_TWO_PI) - 1.0f) * TWO_PI_F;
    }
  }
  return angle;
}

} // namespace FastMath
} // namespace TwinSimulation
