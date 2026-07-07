// PI.h - Generic PI Controller with anti-windup clamping
#pragma once

class PIController {
 public:
  // Kp: Proportional gain
  // Ki: Integral gain
  // outLimit: Symmetric output saturation limit (+/- outLimit)
  PIController(float Kp, float Ki, float outLimit);

  // Reset integrator to zero (call on mode transitions)
  void reset();

  // Run one step of the PI controller.
  // error = reference - measured value
  // dt    = time step in seconds
  // Returns the saturated output.
  float update(float error, float dt);

 private:
  float kp_;
  float ki_;
  float limit_;
  float integrator_;
};
