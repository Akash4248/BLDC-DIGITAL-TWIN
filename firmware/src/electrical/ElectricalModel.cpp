#include "ElectricalModel.h"
#include "../utils/FastMath.h"

namespace TwinSimulation {

constexpr float TWO_PI_OVER_THREE = 2.094395102f;

ElectricalModel::ElectricalModel() {
  memset(&state_, 0, sizeof(state_));
  memset(&config_, 0, sizeof(config_));
}

void ElectricalModel::begin(const ElectricalConfig& config) {
  config_ = config;
  memset(&state_, 0, sizeof(state_));
}

void ElectricalModel::setConfig(const ElectricalConfig& config) {
  config_ = config;
}

void ElectricalModel::update(float va, float vb, float vc, float rotorSpeedMech, float rotorAngleMech, float dt) {
  // Prevent division by zero if inductance is 0 (unconfigured)
  if (config_.Ls <= 0.0f) {
    return; 
  }

  // 1. Calculate Electrical Angle and Speed
  float theta_e = rotorAngleMech * config_.polePairs;
  float omega_e = rotorSpeedMech * config_.polePairs;

  // 2. Calculate Back-EMF (Sinusoidal PMSM profile)
  // Ea = Ke * omega_e * -sin(theta_e)
  // Eb = Ke * omega_e * -sin(theta_e - 120deg)
  // Ec = Ke * omega_e * -sin(theta_e + 120deg)
  float amp = config_.Ke * omega_e;
  state_.ea = amp * -FastMath::sin(theta_e);
  state_.eb = amp * -FastMath::sin(theta_e - TWO_PI_OVER_THREE);
  state_.ec = amp * -FastMath::sin(theta_e + TWO_PI_OVER_THREE);

  // 3. Calculate Neutral Voltage (Assuming star-connected, balanced motor, Ia+Ib+Ic=0)
  // Vn = (Va + Vb + Vc) / 3
  float vn = (va + vb + vc) / 3.0f;

  // 4. Calculate Phase-to-Neutral Voltages
  state_.van = va - vn;
  state_.vbn = vb - vn;
  state_.vcn = vc - vn;

  // 5. Euler Integration for Phase Currents
  // dIa/dt = (Van - R*Ia - Ea) / Ls
  float di_a_dt = (state_.van - (config_.Rs * state_.ia) - state_.ea) / config_.Ls;
  float di_b_dt = (state_.vbn - (config_.Rs * state_.ib) - state_.eb) / config_.Ls;
  
  state_.ia += di_a_dt * dt;
  state_.ib += di_b_dt * dt;
  
  // Kirchhoff's Current Law ensures the sum of currents is zero
  state_.ic = -(state_.ia + state_.ib);
}

const ElectricalState& ElectricalModel::getState() const {
  return state_;
}

const ElectricalConfig& ElectricalModel::getConfig() const {
  return config_;
}

} // namespace TwinSimulation
