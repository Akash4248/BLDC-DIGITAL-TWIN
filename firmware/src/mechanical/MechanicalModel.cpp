#include "MechanicalModel.h"
#include <math.h>

namespace TwinSimulation {

constexpr float TWO_PI_F = 6.28318530718f;

MechanicalModel::MechanicalModel() : currentLoadTorque_(0.0f) {
  memset(&state_, 0, sizeof(state_));
  memset(&config_, 0, sizeof(config_));
}

void MechanicalModel::begin(const MechanicalConfig& config) {
  config_ = config;
  currentLoadTorque_ = 0.0f;
  memset(&state_, 0, sizeof(state_));
}

void MechanicalModel::setConfig(const MechanicalConfig& config) {
  config_ = config;
}

void MechanicalModel::setLoadTorque(float loadTorque) {
  currentLoadTorque_ = loadTorque;
}

float MechanicalModel::getLoadTorque() const {
  return currentLoadTorque_;
}

float MechanicalModel::wrapAngle(float angle) {
  // Efficiently wrap angle to 0 <= angle < 2PI
  while (angle >= TWO_PI_F) angle -= TWO_PI_F;
  while (angle < 0.0f) angle += TWO_PI_F;
  return angle;
}

void MechanicalModel::update(float electromagneticTorque, float loadTorque, float dt) {
  // Prevent division by zero if inertia is unconfigured
  if (config_.inertia <= 0.0f) {
    return;
  }

  // Allow external load torque override, or parameter passed in
  float netLoad = loadTorque + currentLoadTorque_;

  // Determine direction of motion for Coulomb friction
  float direction = 0.0f;
  if (state_.rotorSpeed > 0.01f) direction = 1.0f;
  else if (state_.rotorSpeed < -0.01f) direction = -1.0f;

  // Calculate Net Torque
  // T_net = Te - Tl - B*omega - Tf*sign(omega)
  float frictionTorque = (config_.frictionB * state_.rotorSpeed) + (config_.frictionC * direction);
  float netTorque = electromagneticTorque - netLoad - frictionTorque;

  // Calculate Acceleration: alpha = T_net / J
  state_.acceleration = netTorque / config_.inertia;

  // Integrate Speed: omega = omega + alpha * dt
  state_.rotorSpeed += state_.acceleration * dt;

  // Check for static friction (stiction) stopping the motor completely 
  // if speed is very low and net torque is lower than coulomb friction.
  if (fabs(state_.rotorSpeed) < 0.01f && fabs(electromagneticTorque - netLoad) <= config_.frictionC) {
    state_.rotorSpeed = 0.0f;
    state_.acceleration = 0.0f;
  }

  // Integrate Position: theta = theta + omega * dt
  state_.rotorAngle += state_.rotorSpeed * dt;
  state_.rotorAngle = wrapAngle(state_.rotorAngle);

  // Update electrical angle
  state_.electricalAngle = wrapAngle(state_.rotorAngle * config_.polePairs);
}

const MechanicalState& MechanicalModel::getState() const {
  return state_;
}

const MechanicalConfig& MechanicalModel::getConfig() const {
  return config_;
}

} // namespace TwinSimulation
