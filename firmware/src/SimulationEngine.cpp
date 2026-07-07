#include "SimulationEngine.h"
#include <math.h>

// Shared register accessed by I2C ISR
extern volatile uint16_t latestRawAngle;

SimulationEngine::SimulationEngine() : state_{} {}

void SimulationEngine::begin() {
  inverter_.begin();
  currentOut_.begin();
  motor_.reset();
  motor_.setLoadTorque(0.0f);
  
  state_ = {};
  state_.temperature = 35.0f;
}

void SimulationEngine::step() {
  // 1. Read Inputs
  inverter_.readGates(state_);
  
  // 2. Decode Inverter Gate States to phase voltages
  inverter_.decode(state_, VDC);
  
  // 3. Electrical & Mechanical execution
  motor_.step(state_, SIM_TS);
  
  // 4. Emulate 3-Phase Hall Sensors based on electrical angle
  // Hall spacing is 120° electrical (2π / 3 = 2.0943951 rad)
  state_.hallA = (sinf(state_.elecAngle) >= 0.0f) ? 1 : 0;
  state_.hallB = (sinf(state_.elecAngle - 2.0943951f) >= 0.0f) ? 1 : 0;
  state_.hallC = (sinf(state_.elecAngle + 2.0943951f) >= 0.0f) ? 1 : 0;

  // 5. Emulate AS5600 12-bit magnetic encoder raw register
  float wrappedAngle = state_.mechAngle;
  state_.encoderRawAngle = (uint16_t)((wrappedAngle / (2.0f * M_PI)) * 4095.0f) & 0x0FFF;
  
  // Copy to shared volatile register for I2C master requests
  latestRawAngle = state_.encoderRawAngle;
  
  // 6. Thermal estimation (Estimated copper loss heat rise)
  float iSqSum = (state_.ia * state_.ia + state_.ib * state_.ib + state_.ic * state_.ic);
  state_.temperature = 35.0f + iSqSum * 0.08f;
  if (state_.temperature > 115.0f) state_.temperature = 115.0f;

  // 7. Write feedback signals to Arduino (Analog DAC & PWM)
  currentOut_.write(state_);
}
