// FOC.cpp - FOC Controller implementation
#include "FOC.h"
#include "Current.h"
#include "Clarke.h"
#include "Park.h"
#include "SVPWM.h"
#include <Arduino.h>
#include <math.h>

FOCController::FOCController()
  : pi_id_(KP_ID,    KI_ID,    MAX_VD),
    pi_iq_(KP_IQ,    KI_IQ,    MAX_VQ),
    pi_speed_(KP_SPEED, KI_SPEED, MAX_IQ_REF),
    theta_(0.0f), id_(0.0f), iq_(0.0f),
    mechSpeedRPM_(0.0f), targetSpeedRad_(0.0f),
    openLoopTheta_(0.0f)
{}

void FOCController::begin() {
  pi_id_.reset();
  pi_iq_.reset();
  pi_speed_.reset();
  initCurrentSensors(); // Calibrate offsets relative to the ESP32's actual output
  svpwm_init();
  svpwm_off();
}

void FOCController::setTargetRPM(float rpm) {
  rpm = constrain(rpm, 0.0f, MAX_TARGET_RPM);
  // Electrical rad/s = mech_rpm * (2pi/60) * pole_pairs
  targetSpeedRad_ = rpm * (2.0f * PI / 60.0f) * (float)POLE_PAIRS;
  mechSpeedRPM_ = rpm; // Store for display
}

float FOCController::getMeasuredRPM() const {
  return mechSpeedRPM_;
}

void FOCController::step(float dt) {

#if OPEN_LOOP_TEST
  // ================================================================
  // OPEN-LOOP TEST MODE
  // Theta is generated internally from targetSpeedRad_.
  // This lets you verify SVPWM waveforms on the logic analyzer
  // BEFORE the ESP32 feedback loop is connected.
  // ================================================================

  // Advance internal theta at the target electrical speed
  openLoopTheta_ += targetSpeedRad_ * dt;
  if (openLoopTheta_ >= 2.0f * PI) openLoopTheta_ -= 2.0f * PI;
  if (openLoopTheta_ < 0.0f)       openLoopTheta_ += 2.0f * PI;
  theta_ = openLoopTheta_;

  if (targetSpeedRad_ < 0.01f) {
    // No target: safe off
    svpwm_off();
    return;
  }

  // Apply a fixed voltage vector Vq=OPEN_LOOP_VQ, Vd=0
  // This generates balanced 3-phase sine waves at the target frequency
  AlphaBetaV vab = inversePark(0.0f, OPEN_LOOP_VQ, theta_);
  svpwm_apply(vab.alpha, vab.beta, SUPPLY_VOLTAGE);

#else
  // ================================================================
  // CLOSED-LOOP FOC MODE (HIL with ESP32)
  // ================================================================

  // --- Step 1: Read electrical angle from ESP32 ---
  // ESP32 maps 0..2π → 0V..3.3V, which appears as 0..675 on 5V Arduino ADC
  int rawTheta = analogRead(PIN_THETA);
  theta_ = (rawTheta / 675.0f) * (2.0f * PI);
  if (theta_ > 2.0f * PI) theta_ = 2.0f * PI;

  // --- Step 2: Read phase currents from ESP32 ---
  PhaseCurrents I = readPhaseCurrents();

  // --- Step 3: Clarke Transform (3-phase → alpha-beta) ---
  AlphaBeta ab = clarke(I.ia, I.ib, I.ic);

  // --- Step 4: Park Transform (alpha-beta → DQ) ---
  DQ dq = park(ab.alpha, ab.beta, theta_);
  id_ = dq.d;
  iq_ = dq.q;

  // --- Step 5: Current PI controllers ---
  // For HIL without speed feedback from ESP32, use a fixed iq_ref
  // (ESP32 drives speed; we just command torque)
  float iq_ref = (targetSpeedRad_ > 0.01f) ? MAX_IQ_REF * 0.3f : 0.0f;

  float vd = pi_id_.update(ID_REF - id_, dt);
  float vq = pi_iq_.update(iq_ref - iq_, dt);

  // --- Step 6: Voltage vector limiting ---
  float vmag = sqrtf(vd * vd + vq * vq);
  if (vmag > MAX_VOLTAGE_VECTOR && vmag > 0.0001f) {
    float scale = MAX_VOLTAGE_VECTOR / vmag;
    vd *= scale;
    vq *= scale;
  }

  // --- Step 7: Inverse Park (DQ → alpha-beta) ---
  AlphaBetaV vab = inversePark(vd, vq, theta_);

  // --- Step 8: SVPWM → 6 gate signals ---
  if (targetSpeedRad_ > 0.01f) {
    svpwm_apply(vab.alpha, vab.beta, SUPPLY_VOLTAGE);
  } else {
    svpwm_off();
  }

#endif // OPEN_LOOP_TEST
}
