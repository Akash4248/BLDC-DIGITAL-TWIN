// Current.cpp - Phase current ADC reading implementation
#include "Current.h"

static float offset_ia = ADC_MIDPOINT;
static float offset_ib = ADC_MIDPOINT;
static float offset_ic = ADC_MIDPOINT;

void initCurrentSensors() {
  Serial.println(F("Calibrating Arduino current ADC offsets..."));
  long sumA = 0;
  long sumB = 0;
  long sumC = 0;
  const int samples = 500;
  for (int i = 0; i < samples; i++) {
    sumA += analogRead(PIN_IA);
    sumB += analogRead(PIN_IB);
    sumC += analogRead(PIN_IC);
    delay(2);
  }
  offset_ia = (float)sumA / samples;
  offset_ib = (float)sumB / samples;
  offset_ic = (float)sumC / samples;
  Serial.print(F("Offsets A:")); Serial.print(offset_ia, 2);
  Serial.print(F(" B:")); Serial.print(offset_ib, 2);
  Serial.print(F(" C:")); Serial.println(offset_ic, 2);
}

PhaseCurrents readPhaseCurrents() {
  PhaseCurrents c;
  // Subtract the measured startup offsets instead of a hardcoded midpoint
  c.ia = (analogRead(PIN_IA) - offset_ia) * ADC_SCALE;
  c.ib = (analogRead(PIN_IB) - offset_ib) * ADC_SCALE;
  c.ic = (analogRead(PIN_IC) - offset_ic) * ADC_SCALE;
  return c;
}
