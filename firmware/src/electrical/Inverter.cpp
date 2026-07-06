#include "Inverter.h"

namespace TwinSimulation {

Inverter::Inverter() : deadTimeUs_(1.0f) {
  clearFaults();
}

void Inverter::begin(float deadTimeUs) {
  deadTimeUs_ = deadTimeUs;
  clearFaults();
}

void Inverter::clearFaults() {
  state_.va = 0.0f;
  state_.vb = 0.0f;
  state_.vc = 0.0f;
  state_.shootThroughFault = false;
  state_.invalidState = false;
}

float Inverter::calculateLegVoltage(bool highGate, bool lowGate, float vdc, bool& shootThrough) {
  if (highGate && lowGate) {
    shootThrough = true;
    return 0.0f; // Short circuit, modeled as 0V or collapsing Vdc
  }
  
  if (highGate) {
    return vdc;
  }
  
  if (lowGate) {
    return 0.0f;
  }
  
  // High-Z state (dead-time or open phase)
  // Current would normally freewheel through diodes.
  // For a basic inverter state, we center it at Vdc / 2 or leave it at 0.
  // A true physics model relies on phase current direction to determine diode conduction voltage.
  // We approximate as Vdc / 2 to emulate floating, or 0.0f depending on the electrical model.
  return vdc / 2.0f; 
}

void Inverter::update(bool ah, bool al, bool bh, bool bl, bool ch, bool cl, float vdc) {
  bool shootThroughA = false;
  bool shootThroughB = false;
  bool shootThroughC = false;

  state_.va = calculateLegVoltage(ah, al, vdc, shootThroughA);
  state_.vb = calculateLegVoltage(bh, bl, vdc, shootThroughB);
  state_.vc = calculateLegVoltage(ch, cl, vdc, shootThroughC);

  if (shootThroughA || shootThroughB || shootThroughC) {
    state_.shootThroughFault = true;
  }

  // Detect invalid states like all gates off for an extended period when they shouldn't be,
  // or logic errors not covered by shoot-through.
  if (!ah && !al && !bh && !bl && !ch && !cl) {
    // This is valid during initialization or full disable, but can be flagged for tracking.
    // We won't strictly call it an invalid state fault here to allow coasting.
  }
}

const InverterState& Inverter::getState() const {
  return state_;
}

} // namespace TwinSimulation
