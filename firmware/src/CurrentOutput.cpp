// CurrentOutput.cpp — Layer 5 Implementation for ESP32
#include "CurrentOutput.h"
#include <Arduino.h>
#include <math.h>

void CurrentOutput::begin() {
  // Config DAC pins (pinMode is optional for DAC but safe)
  pinMode(PIN_IA_OUT, OUTPUT);
  pinMode(PIN_IB_OUT, OUTPUT);

  // Attach LEDC PWM to remaining outputs (ESP32 Arduino Core LEDC API)
  ledcAttach(PIN_IC_OUT, LEDC_FREQ, LEDC_BITS);
  ledcAttach(PIN_THETA_OUT, LEDC_FREQ, LEDC_BITS);

  // Initialize all to mid-scale or zero
  dacWrite(PIN_IA_OUT, 127);
  dacWrite(PIN_IB_OUT, 127);
  ledcWrite(PIN_IC_OUT, 127);
  ledcWrite(PIN_THETA_OUT, 0);
}

void CurrentOutput::write(const MotorState& state) {
  // Scale currents from -CURRENT_PEAK_A..+CURRENT_PEAK_A to 0..255
  float scale = 127.5f / CURRENT_PEAK_A;

  int da = (int)(state.ia * scale + 127.5f);
  int db = (int)(state.ib * scale + 127.5f);
  int dc = (int)(state.ic * scale + 127.5f);

  da = constrain(da, 0, 255);
  db = constrain(db, 0, 255);
  dc = constrain(dc, 0, 255);

  // Phase A and B go to actual DACs
  dacWrite(PIN_IA_OUT, da);
  dacWrite(PIN_IB_OUT, db);

  // Phase C goes to LEDC PWM (needs external RC filter)
  ledcWrite(PIN_IC_OUT, dc);

  // Map 0..2π to 0..255
  float normalized = state.elecAngle / (2.0f * M_PI);
  int duty = (int)(normalized * 255.0f);
  duty = constrain(duty, 0, 255);

  // Theta output goes to LEDC PWM (needs external RC filter)
  ledcWrite(PIN_THETA_OUT, duty);
}
