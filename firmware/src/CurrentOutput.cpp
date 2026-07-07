// CurrentOutput.cpp - Analog feedback output from ESP32 to Arduino
// Uses LEDC PWM + external RC low-pass filter to create smooth analog voltages.
// The RC filter cutoff must be well above the motor's electrical frequency
// but well below the PWM carrier frequency (39 kHz).
// Recommended RC: R=1kΩ, C=100nF → fc = 1600 Hz
#include "CurrentOutput.h"
#include <Arduino.h>
#include <math.h>

void CurrentOutput::begin() {
  // Attach LEDC PWM to all 4 output pins (ESP32 Arduino Core v3 API)
  ledcAttach(PIN_IA_OUT,    LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttach(PIN_IB_OUT,    LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttach(PIN_IC_OUT,    LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttach(PIN_THETA_OUT, LEDC_FREQ, LEDC_RESOLUTION);

  // Initialize all outputs to midpoint (represents 0A and 0 angle)
  ledcWrite(PIN_IA_OUT,    127);
  ledcWrite(PIN_IB_OUT,    127);
  ledcWrite(PIN_IC_OUT,    127);
  ledcWrite(PIN_THETA_OUT, 0);
}

void CurrentOutput::writeCurrent(float ia, float ib, float ic) {
  // Map -CURRENT_PEAK_A..+CURRENT_PEAK_A -> 0..255 (mid = 127 = 0A)
  const float scale = 127.5f / CURRENT_PEAK_A;

  int da = (int)(ia * scale + 127.5f);
  int db = (int)(ib * scale + 127.5f);
  int dc = (int)(ic * scale + 127.5f);

  ledcWrite(PIN_IA_OUT, constrain(da, 0, 255));
  ledcWrite(PIN_IB_OUT, constrain(db, 0, 255));
  ledcWrite(PIN_IC_OUT, constrain(dc, 0, 255));
}

void CurrentOutput::writeTheta(float theta_rad) {
  // Map 0..2pi -> 0..255
  float normalized = theta_rad / (2.0f * M_PI);
  int duty = (int)(normalized * 255.0f);
  ledcWrite(PIN_THETA_OUT, constrain(duty, 0, 255));
}
