// SVPWM.cpp - Space Vector PWM, 6-channel complementary output
//
// CRITICAL: All 3 timers MUST run at the same carrier frequency.
// Timer0: Controls millis()/micros(). Its prescaler CANNOT change.
//   → Timer0 Fast PWM, Prescaler=64 → 16MHz/64/256 = 976 Hz
// Timer1 & Timer2: We match Timer0 exactly.
//   → Timer1 Fast PWM 8-bit, Prescaler=64 → 976 Hz
//   → Timer2 Fast PWM,       Prescaler=64 → 976 Hz
//
// Pin assignment:
//   Timer2:  D3 = AH (OC2B, non-inv)  |  D11 = AL (OC2A, inverted)
//   Timer0:  D6 = BH (OC0A, non-inv)  |  D5  = BL (OC0B, inverted)
//   Timer1:  D9 = CH (OC1A, non-inv)  |  D10 = CL (OC1B, inverted)
//
// Dead-time: 3 timer-counts (~3 µs at 976 Hz, 3/256 ≈ 1.2% duty reduction)

#include "SVPWM.h"
#include <avr/io.h>
#include <math.h>

static const uint8_t DT = 3;  // Dead-time counts (prevents shoot-through)

// ---------------------------------------------------------------
// svpwm_init()
// Configures all 3 hardware timers for synchronized 976 Hz Fast PWM
// with complementary (inverted) outputs on each timer.
// Called once from setup().
// ---------------------------------------------------------------
void svpwm_init() {
  // Set all 6 pins as outputs first
  pinMode(PIN_AH, OUTPUT); pinMode(PIN_AL, OUTPUT);
  pinMode(PIN_BH, OUTPUT); pinMode(PIN_BL, OUTPUT);
  pinMode(PIN_CH, OUTPUT); pinMode(PIN_CL, OUTPUT);

  noInterrupts();

  // === Timer 2 — Phase A (D3=AH, D11=AL) ===
  // Fast PWM (WGM20, WGM21), TOP=0xFF
  // OC2B (D3,  AH): COM2B1 only        → non-inverting
  // OC2A (D11, AL): COM2A1|COM2A0      → inverting (complement of AH)
  // Prescaler=64 (CS22|CS21) → 16MHz/64/256 = 976 Hz   ← MATCHES Timer0
  TCCR2A = _BV(COM2A1) | _BV(COM2A0)   // OC2A: inverting
          | _BV(COM2B1)                  // OC2B: non-inverting
          | _BV(WGM21)  | _BV(WGM20);   // Fast PWM
  TCCR2B = _BV(CS22) | _BV(CS21);      // Prescaler = 64

  // === Timer 0 — Phase B (D6=BH, D5=BL) ===
  // Timer0 MUST keep Prescaler=64 (CS01|CS00) — micros() depends on it!
  // OC0A (D6, BH): COM0A1 only         → non-inverting
  // OC0B (D5, BL): COM0B1|COM0B0       → inverting
  TCCR0A = _BV(COM0A1)                  // OC0A: non-inverting
          | _BV(COM0B1) | _BV(COM0B0)   // OC0B: inverting
          | _BV(WGM01)  | _BV(WGM00);   // Fast PWM
  TCCR0B = _BV(CS01) | _BV(CS00);      // Prescaler = 64 (DO NOT CHANGE)

  // === Timer 1 — Phase C (D9=CH, D10=CL) ===
  // Fast PWM 8-bit (WGM10, WGM12), TOP=0xFF
  // OC1A (D9,  CH): COM1A1 only        → non-inverting
  // OC1B (D10, CL): COM1B1|COM1B0      → inverting
  // Prescaler=64 (CS11|CS10) → 16MHz/64/256 = 976 Hz   ← MATCHES Timer0
  TCCR1A = _BV(COM1A1)                  // OC1A: non-inverting
          | _BV(COM1B1) | _BV(COM1B0)   // OC1B: inverting
          | _BV(WGM10);                  // Fast PWM 8-bit (with WGM12 in TCCR1B)
  TCCR1B = _BV(WGM12)                   // Fast PWM 8-bit
          | _BV(CS11) | _BV(CS10);      // Prescaler = 64

  // === Synchronize all 3 timers ===
  // Halt all prescalers, zero all counters, then release together
  // so rising edges are perfectly aligned across all phases.
  GTCCR = _BV(TSM) | _BV(PSRASY) | _BV(PSRSYNC);
  TCNT0 = 0;
  TCNT1 = 0;
  TCNT2 = 0;
  GTCCR = 0;  // Release: all three start counting simultaneously

  // Safe initial state: all high-side OFF
  OCR2B = 0; OCR2A = 255;  // AH=0, AL=inverted(255)→0
  OCR0A = 0; OCR0B = 255;  // BH=0, BL=inverted(255)→0
  OCR1AL = 0; OCR1BL = 255; // CH=0, CL=inverted(255)→0

  interrupts();
}

// ---------------------------------------------------------------
// svpwm_apply(valpha, vbeta, vdc)
// Given the alpha-beta voltage vector, computes 3-phase SVPWM
// duty cycles using midpoint injection and writes them to the
// 6 hardware PWM registers directly.
// ---------------------------------------------------------------
void svpwm_apply(float valpha, float vbeta, float vdc) {
  // Step 1: Inverse Clarke — alpha-beta to three-phase
  float Va = valpha;
  float Vb = -0.5f * valpha + 0.8660254f * vbeta;
  float Vc = -0.5f * valpha - 0.8660254f * vbeta;

  // Step 2: SVPWM midpoint injection
  // Removes common-mode, doubles DC utilization vs SPWM
  float vmax = max(Va, max(Vb, Vc));
  float vmin = min(Va, min(Vb, Vc));
  float voff = 0.5f * (vmax + vmin);
  Va -= voff;
  Vb -= voff;
  Vc -= voff;

  // Step 3: Voltage → duty cycle (0..255 centered at 127 = 0V)
  // Clamp ensures we stay within modulation range
  float inv_half_vdc = 255.0f / vdc;
  int16_t da = (int16_t)(Va * inv_half_vdc + 127.5f);
  int16_t db = (int16_t)(Vb * inv_half_vdc + 127.5f);
  int16_t dc = (int16_t)(Vc * inv_half_vdc + 127.5f);

  da = constrain(da, 0, 255);
  db = constrain(db, 0, 255);
  dc = constrain(dc, 0, 255);

  // Step 4: Apply dead-time to high-side (shrink high-side by DT counts)
  // The inverted low-side automatically has dead-time built in.
  uint8_t ah = (uint8_t)constrain(da - DT, 0, 255);
  uint8_t bh = (uint8_t)constrain(db - DT, 0, 255);
  uint8_t ch = (uint8_t)constrain(dc - DT, 0, 255);

  // Step 5: Write directly to OCR registers with dead-time offset on compare match
  OCR2B  = ah;   // D3  = AH (non-inverting)
  OCR2A  = (uint8_t)constrain((int16_t)ah + DT, 0, 255); // D11 = AL (inverting, offset by DT)

  OCR0A  = bh;   // D6  = BH (non-inverting)
  OCR0B  = (uint8_t)constrain((int16_t)bh + DT, 0, 255); // D5  = BL (inverting, offset by DT)

  OCR1AL = ch;   // D9  = CH (non-inverting)
  OCR1BL = (uint8_t)constrain((int16_t)ch + DT, 0, 255); // D10 = CL (inverting, offset by DT)
}

// ---------------------------------------------------------------
// svpwm_off()
// Forces all high-side outputs LOW and all low-side LOW.
// Safe tri-state for mode transitions and fault handling.
// ---------------------------------------------------------------
void svpwm_off() {
  OCR2B = 0;   OCR2A  = 255;
  OCR0A = 0;   OCR0B  = 255;
  OCR1AL = 0;  OCR1BL = 255;
}
