#include "DigitalTwin.h"
#include "../config.h"

namespace TwinSimulation {

DigitalTwin::DigitalTwin(TwinProtocol::TwinLink& link) : link_(link) {
  diagMaxExecTime_ = 0;
  diagMissedDeadlines_ = 0;
}

void DigitalTwin::begin() {
  applyDefaultConfig();
}

void DigitalTwin::applyDefaultConfig() {
  // Default Electrical config (Generic small BLDC)
  ElectricalConfig eCfg;
  eCfg.Rs = Config::MOTOR_RS;          
  eCfg.Ls = Config::MOTOR_LS;        
  eCfg.Ke = Config::MOTOR_KE;        
  eCfg.polePairs = Config::MOTOR_POLE_PAIRS;
  electrical_.begin(eCfg);

  // Default Mechanical config
  MechanicalConfig mCfg;
  mCfg.inertia = Config::MOTOR_INERTIA;
  mCfg.frictionB = Config::MOTOR_FRICTION;
  mCfg.frictionC = Config::MOTOR_COULOMB;
  mCfg.polePairs = Config::MOTOR_POLE_PAIRS;
  mechanical_.begin(mCfg);

  // Default Sensor configs
  HallConfig hCfg;
  hCfg.placementOffset = 0.0f;
  hCfg.sensorSpacing = Config::HALL_SPACING_RAD;
  hall_.begin(hCfg);

  EncoderConfig encCfg;
  encCfg.ppr = 36; // Lower PPR (36) to prevent aliasing at 10kHz simulation loops
  encCfg.indexOffset = 0.0f;
  encoder_.begin(encCfg);

  CurrentSensorConfig csCfg;
  csCfg.gain = Config::CURRENT_SENSOR_GAIN;
  csCfg.offset = Config::CURRENT_SENSOR_OFFSET;
  csCfg.noiseStdDev = 0.02f;
  csCfg.filterAlpha = 0.5f;
  csCfg.adcBits = Config::ADC_BITS;
  csCfg.adcReference = Config::ADC_VREF;
  
  currentA_.begin(csCfg);
  currentB_.begin(csCfg);
  currentC_.begin(csCfg);

  // Default Thermal config
  ThermalConfig thCfg;
  thCfg.ambientTemp = 25.0f;
  thCfg.cThCu = 50.0f;
  thCfg.rThCuAmb = 2.0f;
  thCfg.cThMag = 150.0f;
  thCfg.rThMagAmb = 5.0f;
  thCfg.rThCuMag = 1.5f;
  thCfg.ironLossCoeff = 0.0001f;
  thermal_.begin(thCfg);
  
  inverter_.begin(1.0f); // 1us deadtime
}

void DigitalTwin::setDiagnostics(uint16_t maxExecTime, uint16_t missedDeadlines) {
  diagMaxExecTime_ = maxExecTime;
  diagMissedDeadlines_ = missedDeadlines;
}

void DigitalTwin::setExternalDutyCycles(float dutyA, float dutyB, float dutyC) {
  extDutyA_ = dutyA;
  extDutyB_ = dutyB;
  extDutyC_ = dutyC;
  useExtDuty_ = true;
}

void DigitalTwin::step(float dt) {
  // 1. Read packet (Non-blocking process incoming serial bytes)
  link_.update();
  
  // Extract inputs from latest telemetry packet from PC dashboard
  const auto& tele = link_.getLatestTelemetry();
  
  // Use Bridge HIL Duty Cycles if provided, otherwise fallback to PC
  float dA = useExtDuty_ ? extDutyA_ : tele.dutyA;
  float dB = useExtDuty_ ? extDutyB_ : tele.dutyB;
  float dC = useExtDuty_ ? extDutyC_ : tele.dutyC;

  // Apply Faults to inputs
  float vdc = faults_.applyVoltageSag(tele.vdc > 0.1f ? tele.vdc : 12.0f);

  // 2. Inverter Model (Using smooth average voltage model)
  inverter_.update(dA, dB, dC, vdc);
  const auto& invState = inverter_.getState();

  // 3. Electrical Model
  const auto& mState = mechanical_.getState();
  electrical_.update(invState.va, invState.vb, invState.vc, 
                     mState.rotorSpeed, mState.rotorAngle, dt);
  const auto& eState = electrical_.getState();

  // 4. Torque Model
  torque_.update(eState, electrical_.getConfig(), mState.rotorSpeed, mState.electricalAngle);
  const auto& tState = torque_.getState();

  // 5. Mechanical Model
  // Apply external mechanical faults (High load, bearing friction, imbalance)
  float loadTorque = faults_.getHighLoadTorque() + 
                     faults_.calculateImbalanceTorque(mState.rotorAngle, mechanical_.getConfig().inertia);
  mechanical_.setConfig({mechanical_.getConfig().inertia, 
                         mechanical_.getConfig().frictionB, 
                         mechanical_.getConfig().frictionC + faults_.getExtraBearingFriction(), 
                         mechanical_.getConfig().polePairs});
  
  float actualTe = faults_.getConfig().lockedRotor ? 0.0f : tState.electromagneticTorque;
  mechanical_.update(actualTe, loadTorque, dt);
  
  // 6. Hall Sensor Model
  hall_.update(mState.electricalAngle);
  const auto& hState = hall_.getState();

  // 7. Encoder Model
  encoder_.update(mState.rotorAngle);
  const auto& encState = encoder_.getState();

  // 8. Current Sensors
  // Apply open phase faults before sensor readings
  float ia_actual = faults_.applyOpenPhaseCurrent(eState.ia, faults_.getConfig().openPhaseA);
  float ib_actual = faults_.applyOpenPhaseCurrent(eState.ib, faults_.getConfig().openPhaseB);
  float ic_actual = faults_.applyOpenPhaseCurrent(eState.ic, faults_.getConfig().openPhaseC);
  
  // Apply sensor output faults
  currentA_.update(faults_.applyCurrentSensorFail(ia_actual, faults_.getConfig().currentSensorAFail));
  currentB_.update(faults_.applyCurrentSensorFail(ib_actual, faults_.getConfig().currentSensorBFail));
  currentC_.update(faults_.applyCurrentSensorFail(ic_actual, faults_.getConfig().currentSensorCFail));

  // 9. Temperature Model
  thermal_.update(eState, electrical_.getConfig(), mState, dt);
  const auto& thermState = thermal_.getState();

  // 10. Fault Injection overrides on outputs
  bool hallAFinal = faults_.applyHallFail(hState.a, faults_.getConfig().hallAFail);
  bool hallBFinal = faults_.applyHallFail(hState.b, faults_.getConfig().hallBFail);
  bool hallCFinal = faults_.applyHallFail(hState.c, faults_.getConfig().hallCFail);
  bool encAFinal = faults_.applyEncoderFail(encState.a);
  
  // 11. Transmit Feedback (Downsampled to 1kHz to avoid saturating UART)
  static int teleCounter = 0;
  if (++teleCounter >= 10) {
    teleCounter = 0;
    TwinProtocol::TwinStatePacket txPacket;
    txPacket.sequence = tele.sequence; 
    
    txPacket.ia = currentA_.getState().analogVoltage; 
    txPacket.ib = currentB_.getState().analogVoltage;
    txPacket.ic = currentC_.getState().analogVoltage;
    
    txPacket.rotorAngle = mState.rotorAngle;
    txPacket.rotorSpeed = mState.rotorSpeed;
    
    txPacket.maxExecTime = diagMaxExecTime_;
    txPacket.missedDeadlines = diagMissedDeadlines_;
    
    link_.sendTwinState(txPacket);
  }
}

} // namespace TwinSimulation
