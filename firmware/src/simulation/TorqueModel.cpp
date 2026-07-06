#include "TorqueModel.h"
#include "../utils/FastMath.h"

namespace TwinSimulation {

constexpr float TWO_PI_OVER_THREE = 2.094395102f;

TorqueModel::TorqueModel() {
  memset(&state_, 0, sizeof(state_));
}

void TorqueModel::update(const ElectricalState& elecState, 
                         const ElectricalConfig& elecConfig,
                         float mechanicalSpeed,
                         float electricalAngle) {
  
  // 1. Compute Electrical Power Input
  // P_in = Van*Ia + Vbn*Ib + Vcn*Ic
  state_.electricalPower = (elecState.van * elecState.ia) + 
                           (elecState.vbn * elecState.ib) + 
                           (elecState.vcn * elecState.ic);

  // 2. Compute Electromagnetic Torque (Te)
  // Te = Kt * (Ia * f(a) + Ib * f(b) + Ic * f(c))
  // Where Kt = Ke * polePairs, and f(theta) is the back-emf shape function.
  float kt = elecConfig.Ke * elecConfig.polePairs;
  float fA = -FastMath::sin(electricalAngle);
  float fB = -FastMath::sin(electricalAngle - TWO_PI_OVER_THREE);
  float fC = -FastMath::sin(electricalAngle + TWO_PI_OVER_THREE);
  
  state_.electromagneticTorque = kt * ((elecState.ia * fA) + 
                                       (elecState.ib * fB) + 
                                       (elecState.ic * fC));

  // 3. Compute Mechanical Power
  state_.mechanicalPower = state_.electromagneticTorque * mechanicalSpeed;

  // 4. Compute Efficiency
  if (state_.electricalPower > 0.5f && state_.mechanicalPower > 0.0f) {
    state_.efficiency = state_.mechanicalPower / state_.electricalPower;
    if (state_.efficiency > 1.0f) state_.efficiency = 1.0f;
  } else if (state_.electricalPower < -0.5f && state_.mechanicalPower < 0.0f) {
    // Regeneration efficiency
    state_.efficiency = state_.electricalPower / state_.mechanicalPower;
    if (state_.efficiency > 1.0f) state_.efficiency = 1.0f;
  } else {
    state_.efficiency = 0.0f;
  }
}

const TorqueState& TorqueModel::getState() const {
  return state_;
}

} // namespace TwinSimulation
