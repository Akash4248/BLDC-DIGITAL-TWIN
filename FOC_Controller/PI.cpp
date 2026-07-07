// PI.cpp - PI Controller implementation with conditional anti-windup
#include "PI.h"

PIController::PIController(float Kp, float Ki, float outLimit)
  : kp_(Kp), ki_(Ki), limit_(outLimit), integrator_(0.0f) {}

void PIController::reset() {
  integrator_ = 0.0f;
}

float PIController::update(float error, float dt) {
  // Proportional term
  float p_term = kp_ * error;
  
  // Tentatively integrate
  float next_integrator = integrator_ + error * dt;
  float i_term = ki_ * next_integrator;
  
  float output = p_term + i_term;
  
  // Anti-windup: only integrate if we are NOT saturated in the same direction as the error
  if (output > limit_) {
    output = limit_;
    // Only update integrator if it would reduce the error (conditional integration)
    if (error < 0.0f) {
      integrator_ = next_integrator;
    }
  } else if (output < -limit_) {
    output = -limit_;
    if (error > 0.0f) {
      integrator_ = next_integrator;
    }
  } else {
    integrator_ = next_integrator;
  }
  
  return output;
}
