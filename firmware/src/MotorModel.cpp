// MotorModel.cpp — Layer 4: PMSM simulation
//
// Electrical equations (surface-mounted PMSM, Ld = Lq = Ls):
//
//   L·(dIa/dt) = Va - R·Ia - Ea
//   L·(dIb/dt) = Vb - R·Ib - Eb
//   L·(dIc/dt) = Vc - R·Ic - Ec
//
// Back-EMF (sinusoidal):
//   Ea = Ke·ωe·cos(θe)
//   Eb = Ke·ωe·cos(θe - 2π/3)
//   Ec = Ke·ωe·cos(θe + 2π/3)
//
// Electromagnetic torque:
//   Te = Kt·[Ia·cos(θe) + Ib·cos(θe-2π/3) + Ic·cos(θe+2π/3)]
//
// Mechanical equations (Newton's 2nd law):
//   J·(dω/dt) = Te - B·ω - TL
//   dθm/dt    = ω
//
// Integration: Forward Euler at SIM_TS = 100 µs

#include "MotorModel.h"
#include <math.h>

static constexpr float TWO_PI_OVER3= 2.09439510f;  // 2π/3

MotorModel::MotorModel()
  : ia_(0.f), ib_(0.f), ic_(0.f),
    omega_(0.f), thetaMech_(0.f), loadTorque_(0.f)
{
}

float MotorModel::wrapAngle(float a) {
  while (a >= TWO_PI) a -= TWO_PI;
  while (a <  0.f)    a += TWO_PI;
  return a;
}

void MotorModel::step(MotorState& state, float dt) {
  if (MOTOR_LS <= 0.0f) {
    return;
  }

  // 1. Calculate Electrical Angle and Speed
  float thetaE = wrapAngle(thetaMech_ * MOTOR_POLE_PAIRS);
  float omegaE = omega_ * MOTOR_POLE_PAIRS;

  // 2. Calculate Back-EMF in phase coordinates (sinusoidal PMSM profile)
  float amp = MOTOR_KE * omegaE;
  float ea = amp * -sinf(thetaE);
  float eb = amp * -sinf(thetaE - TWO_PI_OVER3);
  float ec = amp * -sinf(thetaE + TWO_PI_OVER3);

  // 3. Calculate Neutral Voltage: Vn = (Va + Vb + Vc) / 3
  float vn = (state.va + state.vb + state.vc) * 0.33333333f;

  // 4. Calculate Phase-to-Neutral Voltages
  float van = state.va - vn;
  float vbn = state.vb - vn;
  float vcn = state.vc - vn;

  // 5. Integrate Phase Currents (Kirchhoff's Current Law: Ia + Ib + Ic = 0)
  float di_a = (van - (MOTOR_RS * ia_) - ea) / MOTOR_LS;
  float di_b = (vbn - (MOTOR_RS * ib_) - eb) / MOTOR_LS;

  ia_ += di_a * dt;
  ib_ += di_b * dt;
  ic_ = -(ia_ + ib_);

  // Guard numerical limits to protect against transient overshoots
  if (ia_ > 50.0f) ia_ = 50.0f;
  if (ia_ < -50.0f) ia_ = -50.0f;
  if (ib_ > 50.0f) ib_ = 50.0f;
  if (ib_ < -50.0f) ib_ = -50.0f;
  if (ic_ > 50.0f) ic_ = 50.0f;
  if (ic_ < -50.0f) ic_ = -50.0f;

  // 6. Calculate Electromagnetic Torque (Te)
  float kt = MOTOR_KE * MOTOR_POLE_PAIRS;
  float fa = -sinf(thetaE);
  float fb = -sinf(thetaE - TWO_PI_OVER3);
  float fc = -sinf(thetaE + TWO_PI_OVER3);
  float Te = kt * (ia_ * fa + ib_ * fb + ic_ * fc);

  // 7. Mechanical model: Forward Euler with Coulomb friction
  float direction = 0.0f;
  if (omega_ > 0.01f) direction = 1.0f;
  else if (omega_ < -0.01f) direction = -1.0f;
  
  float frictionTorque = (MOTOR_B * omega_) + (0.005f * direction);
  float netTorque = Te - loadTorque_ - frictionTorque;

  float dOmega = netTorque / MOTOR_J;
  omega_ += dOmega * dt;

  // Limit mechanical speed for numerical integrity (15000 RPM ≈ 1570 rad/s)
  if (omega_ > 1570.0f) omega_ = 1570.0f;
  if (omega_ < -1570.0f) omega_ = -1570.0f;

  thetaMech_ = wrapAngle(thetaMech_ + omega_ * dt);

  // 8. Update public state fields
  state.ia          = ia_;
  state.ib          = ib_;
  state.ic          = ic_;
  state.torqueElec  = Te;
  state.mechSpeedRad= omega_;
  state.mechSpeedRPM= omega_ * 60.f / TWO_PI;
  state.mechAngle   = thetaMech_;
  state.elecAngle   = wrapAngle(thetaMech_ * MOTOR_POLE_PAIRS);
}

void MotorModel::reset() {
  ia_ = 0.0f;
  ib_ = 0.0f;
  ic_ = 0.0f;
  omega_ = 0.0f;
  thetaMech_ = 0.0f;
}
