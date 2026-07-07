// Inverter.cpp - 3-phase full-bridge inverter model implementation
#include "Inverter.h"

void Inverter::update(float dutyA, float dutyB, float dutyC, float vdc) {
  // Continuous average voltage model:
  // phase voltage = duty * Vdc - Vn (neutral point)
  // Vn = (Va_raw + Vb_raw + Vc_raw) / 3  (removes common mode)
  float va_raw = dutyA * vdc;
  float vb_raw = dutyB * vdc;
  float vc_raw = dutyC * vdc;

  float vn = (va_raw + vb_raw + vc_raw) / 3.0f;

  state_.va = va_raw - vn;
  state_.vb = vb_raw - vn;
  state_.vc = vc_raw - vn;
}
