#pragma once

#include <Arduino.h>
#include "../electrical/ElectricalModel.h"
#include "../mechanical/MechanicalModel.h"

namespace TwinSimulation {

struct ThermalConfig {
  float ambientTemp;      // Ambient temperature (Celsius)
  
  // Copper (Stator Windings)
  float cThCu;            // Thermal capacitance of copper (J/K)
  float rThCuAmb;         // Thermal resistance copper to ambient (K/W)
  
  // Magnets (Rotor)
  float cThMag;           // Thermal capacitance of magnets (J/K)
  float rThMagAmb;        // Thermal resistance magnet to ambient (K/W)
  
  // Cross coupling
  float rThCuMag;         // Thermal resistance copper to magnet (K/W)
  
  // Iron loss coefficient
  float ironLossCoeff;    // W / (rad/s)^2
};

struct ThermalState {
  float copperTemp;       // Current copper temperature (Celsius)
  float magnetTemp;       // Current magnet temperature (Celsius)
};

class ThermalModel {
 public:
  ThermalModel();

  // Initialize with configuration
  void begin(const ThermalConfig& config);

  // Update thermal simulation
  // elecState: Provides phase currents for copper loss (I^2 * R)
  // elecConfig: Provides Rs for copper loss
  // mechState: Provides speed for iron loss
  // dt: Time step for Euler integration (seconds)
  void update(const ElectricalState& elecState, 
              const ElectricalConfig& elecConfig,
              const MechanicalState& mechState,
              float dt);

  // Read the calculated states
  const ThermalState& getState() const;
  
  // Set configuration at runtime
  void setConfig(const ThermalConfig& config);
  
  // Force reset temperatures (e.g. debugging)
  void resetTemperatures(float temp);

 private:
  ThermalState state_;
  ThermalConfig config_;
};

} // namespace TwinSimulation
