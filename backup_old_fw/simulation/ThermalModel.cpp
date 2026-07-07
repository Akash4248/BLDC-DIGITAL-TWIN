#include "ThermalModel.h"

namespace TwinSimulation {

ThermalModel::ThermalModel() {
  memset(&state_, 0, sizeof(state_));
  
  // Reasonable defaults for a small BLDC motor
  config_.ambientTemp = 25.0f;
  config_.cThCu = 50.0f;      // J/K
  config_.rThCuAmb = 2.0f;    // K/W
  config_.cThMag = 150.0f;    // J/K (Rotor has more mass but harder to cool)
  config_.rThMagAmb = 5.0f;   // K/W
  config_.rThCuMag = 1.5f;    // K/W (Coupling between stator and rotor)
  config_.ironLossCoeff = 0.0001f; 

  state_.copperTemp = config_.ambientTemp;
  state_.magnetTemp = config_.ambientTemp;
}

void ThermalModel::begin(const ThermalConfig& config) {
  config_ = config;
  state_.copperTemp = config_.ambientTemp;
  state_.magnetTemp = config_.ambientTemp;
}

void ThermalModel::setConfig(const ThermalConfig& config) {
  config_ = config;
}

void ThermalModel::resetTemperatures(float temp) {
  state_.copperTemp = temp;
  state_.magnetTemp = temp;
}

void ThermalModel::update(const ElectricalState& elecState, 
                          const ElectricalConfig& elecConfig,
                          const MechanicalState& mechState,
                          float dt) {
  
  // 1. Calculate Heat Sources (Power Losses)
  
  // Copper loss: P_cu = I_a^2*R + I_b^2*R + I_c^2*R
  float pCu = (elecState.ia * elecState.ia + 
               elecState.ib * elecState.ib + 
               elecState.ic * elecState.ic) * elecConfig.Rs;
               
  // Iron loss (Eddy currents & Hysteresis approximated by speed squared)
  float pFe = config_.ironLossCoeff * (mechState.rotorSpeed * mechState.rotorSpeed);
  
  // 2. Calculate Heat Transfers (Conduction & Convection)
  
  // Convection to ambient
  float qCuAmb = (state_.copperTemp - config_.ambientTemp) / config_.rThCuAmb;
  float qMagAmb = (state_.magnetTemp - config_.ambientTemp) / config_.rThMagAmb;
  
  // Conduction between Stator (Copper) and Rotor (Magnet) across air gap
  float qCuMag = (state_.copperTemp - state_.magnetTemp) / config_.rThCuMag;
  
  // 3. Integrate Temperatures using Euler Method
  // dT/dt = (Power_In - Power_Out) / Thermal_Capacitance
  
  if (config_.cThCu > 0.0f) {
    float dTempCuDt = (pCu - qCuAmb - qCuMag) / config_.cThCu;
    state_.copperTemp += dTempCuDt * dt;
  }
  
  if (config_.cThMag > 0.0f) {
    float dTempMagDt = (pFe - qMagAmb + qCuMag) / config_.cThMag;
    state_.magnetTemp += dTempMagDt * dt;
  }
}

const ThermalState& ThermalModel::getState() const {
  return state_;
}

} // namespace TwinSimulation
