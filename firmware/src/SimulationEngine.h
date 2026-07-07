#pragma once
#include "Config.h"
#include "MotorState.h"
#include "Inverter.h"
#include "MotorModel.h"
#include "CurrentOutput.h"

class SimulationEngine {
 public:
  SimulationEngine();
  void begin();
  
  // Advance simulation by 1 step (100 µs fixed timestep)
  void step();

  const MotorState& getState() const { return state_; }

 private:
  MotorState state_;
  Inverter inverter_;
  MotorModel motor_;
  CurrentOutput currentOut_;
};
