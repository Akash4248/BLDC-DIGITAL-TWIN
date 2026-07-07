// Inverter.cpp — Layer 2 & 3 implementation with 6 gate inputs
#include "Inverter.h"

void Inverter::begin() {
  pinMode(PIN_AH, INPUT);
  pinMode(PIN_AL, INPUT);
  pinMode(PIN_BH, INPUT);
  pinMode(PIN_BL, INPUT);
  pinMode(PIN_CH, INPUT);
  pinMode(PIN_CL, INPUT);
}

// Layer 2: Read instantaneous gate states
void Inverter::readGates(MotorState& state) {
  state.ah = digitalRead(PIN_AH);
  state.bh = digitalRead(PIN_BH);
  state.ch = digitalRead(PIN_CH);

#if USE_3PIN_BRIDGE
  // Synthesize complementary low-side gates automatically in software
  state.al = !state.ah;
  state.bl = !state.bh;
  state.cl = !state.ch;
#else
  // Read all 6 inputs physically
  state.al = digitalRead(PIN_AL);
  state.bl = digitalRead(PIN_BL);
  state.cl = digitalRead(PIN_CL);
#endif
}

// Layer 3: Decode gate states → phase-to-neutral voltages
void Inverter::decode(MotorState& state, float vdc) {
  float v_half = vdc * 0.5f;

  float va_mid = 0.0f;
  if (state.ah && !state.al)      va_mid = v_half;
  else if (!state.ah && state.al) va_mid = -v_half;

  float vb_mid = 0.0f;
  if (state.bh && !state.bl)      vb_mid = v_half;
  else if (!state.bh && state.bl) vb_mid = -v_half;

  float vc_mid = 0.0f;
  if (state.ch && !state.cl)      vc_mid = v_half;
  else if (!state.ch && state.cl) vc_mid = -v_half;

  // Subtract neutral point voltage
  float vn = (va_mid + vb_mid + vc_mid) * 0.33333333f;

  state.va = va_mid - vn;
  state.vb = vb_mid - vn;
  state.vc = vc_mid - vn;
}
