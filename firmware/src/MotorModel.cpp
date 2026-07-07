// MotorModel.cpp - Complete PMSM/BLDC motor simulation
// Uses Forward Euler integration for electrical and mechanical equations.
//
// Electrical model (surface-mounted PMSM with Ld=Lq=Ls):
//   L * d(ia)/dt = va - Rs*ia - Ea
//   L * d(ib)/dt = vb - Rs*ib - Eb
//   L * d(ic)/dt = vc - Rs*ic - Ec
//
// Back-EMF:
//   Ea = Ke * we * cos(theta_e)
//   Eb = Ke * we * cos(theta_e - 2pi/3)
//   Ec = Ke * we * cos(theta_e + 2pi/3)
//
// Mechanical model:
//   J * d(omega_m)/dt = Te - B*omega_m - TL
//   d(theta_m)/dt     = omega_m
//
// Electromagnetic torque:
//   Te = Kt * (ia*cos(theta_e) + ib*cos(theta_e - 2pi/3) + ic*cos(theta_e + 2pi/3))
//      = (3/2) * Kt * iq  (equivalent DQ form)

#include "MotorModel.h"
#include <math.h>

MotorModel::MotorModel()
  : loadTorque_(0.0f), ia_(0.0f), ib_(0.0f), ic_(0.0f),
    mechSpeed_(0.0f), mechAngle_(0.0f) {
  state_ = {};
}

float MotorModel::wrapAngle(float angle) {
  while (angle >= 2.0f * M_PI) angle -= 2.0f * M_PI;
  while (angle < 0.0f)         angle += 2.0f * M_PI;
  return angle;
}

void MotorModel::step(float va, float vb, float vc, float dt) {
  // Current electrical angle
  float theta_e = wrapAngle(mechAngle_ * MOTOR_POLE_PAIRS);

  // Electrical speed (rad/s)
  float omega_e = mechSpeed_ * MOTOR_POLE_PAIRS;

  // --- Back-EMF computation ---
  // Ea, Eb, Ec are induced voltages opposing current flow
  float Ea = MOTOR_KE * omega_e * cosf(theta_e);
  float Eb = MOTOR_KE * omega_e * cosf(theta_e - 2.0f * M_PI / 3.0f);
  float Ec = MOTOR_KE * omega_e * cosf(theta_e + 2.0f * M_PI / 3.0f);

  // --- Electrical model: Forward Euler integration of phase currents ---
  // di/dt = (v - Rs*i - Emf) / Ls
  float dia = (va - MOTOR_RS * ia_ - Ea) / MOTOR_LS;
  float dib = (vb - MOTOR_RS * ib_ - Eb) / MOTOR_LS;
  float dic = (vc - MOTOR_RS * ic_ - Ec) / MOTOR_LS;

  ia_ += dia * dt;
  ib_ += dib * dt;
  ic_ += dic * dt;

  // --- Electromagnetic torque ---
  // Te = Kt * (ia*cos(theta_e) + ib*cos(theta_e-2pi/3) + ic*cos(theta_e+2pi/3))
  float Te = MOTOR_KT * (
      ia_ * cosf(theta_e) +
      ib_ * cosf(theta_e - 2.0f * M_PI / 3.0f) +
      ic_ * cosf(theta_e + 2.0f * M_PI / 3.0f)
  );

  // --- Mechanical model: Forward Euler integration ---
  // d(omega_m)/dt = (Te - B*omega_m - TL) / J
  float dOmega = (Te - MOTOR_B * mechSpeed_ - loadTorque_) / MOTOR_J;
  mechSpeed_ += dOmega * dt;

  // d(theta_m)/dt = omega_m
  mechAngle_ += mechSpeed_ * dt;
  mechAngle_ = wrapAngle(mechAngle_);

  // --- Update public state struct ---
  state_.ia = ia_;
  state_.ib = ib_;
  state_.ic = ic_;
  state_.torqueElec    = Te;
  state_.mechAngle     = mechAngle_;
  state_.elecAngle     = wrapAngle(mechAngle_ * MOTOR_POLE_PAIRS);
  state_.rotorSpeedRad = mechSpeed_;
  state_.rotorSpeedRPM = mechSpeed_ * 60.0f / (2.0f * M_PI);
}
