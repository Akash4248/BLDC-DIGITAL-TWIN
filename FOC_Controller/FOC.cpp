// FOC.cpp - Top-level FOC algorithm implementation
#include "FOC.h"
#include "Current.h"
#include "Clarke.h"
#include "Park.h"
#include "SVPWM.h"
#include <Arduino.h>
#include <math.h>

FOCController::FOCController()
  : pi_id_(KP_ID, KI_ID, MAX_VD),
    pi_iq_(KP_IQ, KI_IQ, MAX_VQ),
    pi_speed_(KP_SPEED, KI_SPEED, MAX_IQ_REF),
    theta_(0.0f), id_(0.0f), iq_(0.0f),
    mechSpeedRPM_(0.0f), targetSpeedRad_(0.0f)
{}

void FOCController::begin() {
  pi_id_.reset();
  pi_iq_.reset();
  pi_speed_.reset();
  svpwm_init();
  svpwm_off();
}

void FOCController::setTargetRPM(float rpm) {
  rpm = constrain(rpm, 0.0f, MAX_TARGET_RPM);
  // Convert RPM to electrical rad/s: omega_elec = omega_mech * pole_pairs
  // omega_mech [rad/s] = rpm * 2*pi/60
  targetSpeedRad_ = rpm * (2.0f * PI / 60.0f) * POLE_PAIRS;
}

float FOCController::getMeasuredRPM() const {
  return mechSpeedRPM_;
}

void FOCController::step(float dt) {
  // --- Step 1: Read rotor angle from ESP32 analog output ---
  // ESP32 maps electrical angle 0..2pi to 0V..5V (ADC 0..1023)
  int rawTheta = analogRead(PIN_THETA);
  theta_ = (rawTheta / 1023.0f) * (2.0f * PI);

  // --- Step 2: Read phase currents from ESP32 ---
  PhaseCurrents I = readPhaseCurrents();

  // --- Step 3: Clarke Transform (3-phase → alpha-beta) ---
  AlphaBeta ab = clarke(I.ia, I.ib, I.ic);

  // --- Step 4: Park Transform (alpha-beta → DQ rotating frame) ---
  DQ dq = park(ab.alpha, ab.beta, theta_);
  id_ = dq.d;
  iq_ = dq.q;

  // --- Step 5: Speed estimation (derive from angle differentiation on ESP32 side,
  //              here we use a rough RPM approximation from the iq output for display) ---
  // (Full speed estimation requires angle history - simple approximation here)
  mechSpeedRPM_ = (targetSpeedRad_ / (POLE_PAIRS * 2.0f * PI / 60.0f));

  // --- Step 6: Speed PI controller → sets iq_ref ---
  float elecSpeedEstRad = targetSpeedRad_; // Use feedforward for HIL (ESP32 drives the real speed)
  float speedError = targetSpeedRad_ - elecSpeedEstRad;
  // For HIL, we directly use a fixed iq_ref (open speed loop), tuned by user.
  // When connected to real motor with tachometer, replace with:
  // float iq_ref = pi_speed_.update(speedError, dt);
  float iq_ref = MAX_IQ_REF * 0.25f; // 25% torque for initial HIL testing

  // --- Step 7: Current PI controllers → compute Vd, Vq ---
  float vd = pi_id_.update(ID_REF - id_, dt);
  float vq = pi_iq_.update(iq_ref - iq_, dt);

  // --- Step 8: Voltage vector limiting ---
  float vmag_sq = vd * vd + vq * vq;
  if (vmag_sq > MAX_VOLTAGE_VECTOR * MAX_VOLTAGE_VECTOR) {
    float scale = MAX_VOLTAGE_VECTOR / sqrtf(vmag_sq);
    vd *= scale;
    vq *= scale;
  }

  // --- Step 9: Inverse Park (DQ → alpha-beta) ---
  AlphaBetaV vab = inversePark(vd, vq, theta_);

  // --- Step 10: SVPWM → write complementary gate signals ---
  svpwm_apply(vab.alpha, vab.beta, SUPPLY_VOLTAGE);
}
