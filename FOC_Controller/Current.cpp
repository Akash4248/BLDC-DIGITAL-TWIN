// Current.cpp - Phase current ADC reading implementation
#include "Current.h"

PhaseCurrents readPhaseCurrents() {
  PhaseCurrents c;
  // Each ADC reading is mapped from 0..1023 to -PEAK..+PEAK
  // 0A corresponds to ADC_MIDPOINT (2.5V on a 5V Arduino)
  c.ia = (analogRead(PIN_IA) - ADC_MIDPOINT) * ADC_SCALE;
  c.ib = (analogRead(PIN_IB) - ADC_MIDPOINT) * ADC_SCALE;
  c.ic = (analogRead(PIN_IC) - ADC_MIDPOINT) * ADC_SCALE;
  return c;
}
