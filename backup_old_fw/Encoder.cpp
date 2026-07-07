// Encoder.cpp - Gate Signal Reader implementation for ESP32
// Uses xthal_get_ccount() (CPU cycle counter at 240 MHz) for
// nanosecond-accurate duty cycle measurement via CHANGE interrupts.
#include "Encoder.h"
#include <xtensa/core-macros.h>

// --- Internal state for each of the 3 high-side signals ---
// Low-side signals are read to compute net duty but AH/BH/CH dominate
static volatile uint32_t gHighTimeA = 0, gPeriodA = 24000, gLastRiseA = 0;
static volatile uint32_t gHighTimeB = 0, gPeriodB = 24000, gLastRiseB = 0;
static volatile uint32_t gHighTimeC = 0, gPeriodC = 24000, gLastRiseC = 0;

// --- ISR handlers: measure high-pulse width and period on the HIGH-side pins ---
static void IRAM_ATTR isrAH() {
  uint32_t now = xthal_get_ccount();
  if (digitalRead(PIN_AH)) {
    gPeriodA  = now - gLastRiseA;
    gLastRiseA = now;
  } else {
    gHighTimeA = now - gLastRiseA;
  }
}

static void IRAM_ATTR isrBH() {
  uint32_t now = xthal_get_ccount();
  if (digitalRead(PIN_BH)) {
    gPeriodB  = now - gLastRiseB;
    gLastRiseB = now;
  } else {
    gHighTimeB = now - gLastRiseB;
  }
}

static void IRAM_ATTR isrCH() {
  uint32_t now = xthal_get_ccount();
  if (digitalRead(PIN_CH)) {
    gPeriodC  = now - gLastRiseC;
    gLastRiseC = now;
  } else {
    gHighTimeC = now - gLastRiseC;
  }
}

void GateSignalReader::begin() {
  pinMode(PIN_AH, INPUT); pinMode(PIN_AL, INPUT);
  pinMode(PIN_BH, INPUT); pinMode(PIN_BL, INPUT);
  pinMode(PIN_CH, INPUT); pinMode(PIN_CL, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_AH), isrAH, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_BH), isrBH, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_CH), isrCH, CHANGE);
}

// Helper: returns duty ratio (0.0..1.0) with stuck-pin detection
static float computeDuty(uint32_t highTime, uint32_t period, int pin) {
  uint32_t now = xthal_get_ccount();
  // If no edge has been seen for 2+ periods (~512us at 976Hz), pin is stuck
  if (now - gLastRiseA > period * 2) {
    return digitalRead(pin) ? 1.0f : 0.0f;
  }
  if (period == 0) return 0.0f;
  float d = (float)highTime / (float)period;
  if (d < 0.0f) d = 0.0f;
  if (d > 1.0f) d = 1.0f;
  return d;
}

float GateSignalReader::getDutyA() const {
  return computeDuty(gHighTimeA, gPeriodA, PIN_AH);
}

float GateSignalReader::getDutyB() const {
  return computeDuty(gHighTimeB, gPeriodB, PIN_BH);
}

float GateSignalReader::getDutyC() const {
  return computeDuty(gHighTimeC, gPeriodC, PIN_CH);
}
