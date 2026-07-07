// SVPWM.cpp - Space Vector PWM implementation
// Outputs 6 complementary hardware PWM signals using Timer0/1/2 registers directly.
// This bypasses analogWrite() for 100% deterministic, jitter-free timing.
#include "SVPWM.h"
#include <avr/io.h>
#include <math.h>

// Dead-time in PWM counts (0-255). 4 counts ~ 1.6us at 976Hz PWM.
// Prevents shoot-through by inserting a brief gap between AH going LOW and AL going HIGH.
static const uint8_t DEAD_TIME = 4;

// ---------------------------------------------------------------
// svpwm_init()
// Configure Timers 0, 1, and 2 for fast PWM at ~31.4 kHz
// Pin Mapping:
//   Timer2 : PIN_AH (D3 = OC2B), PIN_AL (D11 = OC2A)
//   Timer0 : PIN_BH (D6 = OC0A), PIN_BL (D5 = OC0B)
//   Timer1 : PIN_CH (D9 = OC1A), PIN_CL (D10 = OC1B)
// ---------------------------------------------------------------
void svpwm_init() {
  // Configure output pins
  pinMode(PIN_AH, OUTPUT); pinMode(PIN_AL, OUTPUT);
  pinMode(PIN_BH, OUTPUT); pinMode(PIN_BL, OUTPUT);
  pinMode(PIN_CH, OUTPUT); pinMode(PIN_CL, OUTPUT);

  noInterrupts();

  // ------- Timer 2: Phase A (D3=AH, D11=AL) -------
  // Fast PWM, prescaler=1 -> 16MHz/256 = 62.5 kHz
  // Using prescaler=8 -> 7.8 kHz (good balance of resolution & frequency)
  TCCR2A = _BV(COM2A1) | _BV(COM2A0)  // OC2A (D11=AL): inverted (complements AH)
         | _BV(COM2B1)                  // OC2B (D3=AH): non-inverted
         | _BV(WGM21) | _BV(WGM20);    // Fast PWM mode
  TCCR2B = _BV(CS21);                  // Prescaler = 8 -> ~7.8 kHz PWM
  OCR2A = 0; OCR2B = 0;

  // ------- Timer 0: Phase B (D6=BH, D5=BL) -------
  // NOTE: Timer0 drives micros()/millis(). We keep its prescaler at 64 (976 Hz PWM)
  //       to not break timing. Lower freq is acceptable for HIL purposes.
  TCCR0A = _BV(COM0A1)                 // OC0A (D6=BH): non-inverted
         | _BV(COM0B1) | _BV(COM0B0)   // OC0B (D5=BL): inverted
         | _BV(WGM01) | _BV(WGM00);    // Fast PWM mode
  TCCR0B = _BV(CS01) | _BV(CS00);     // Prescaler = 64 -> 976 Hz (keeps micros working)
  OCR0A = 0; OCR0B = 255;              // BH off, BL on (safe state)

  // ------- Timer 1: Phase C (D9=CH, D10=CL) -------
  // Fast PWM 8-bit, prescaler=8 -> ~7.8 kHz
  TCCR1A = _BV(COM1A1)                 // OC1A (D9=CH): non-inverted
         | _BV(COM1B1) | _BV(COM1B0)   // OC1B (D10=CL): inverted
         | _BV(WGM10);                 // Fast PWM 8-bit (TOP=0xFF)
  TCCR1B = _BV(WGM12) | _BV(CS11);   // Fast PWM, prescaler=8
  OCR1A = 0; OCR1B = 255;             // CH off, CL on (safe state)

  // ------- Synchronize all 3 timers -------
  GTCCR = _BV(TSM) | _BV(PSRASY) | _BV(PSRSYNC); // Halt all prescalers
  TCNT0 = 0; TCNT1 = 0; TCNT2 = 0;                // Zero all counters
  GTCCR = 0;                                        // Release, all start together

  interrupts();
}

// ---------------------------------------------------------------
// svpwm_apply()
// Given Valpha and Vbeta (in Volts), compute duty cycles using
// Space Vector modulation with midpoint injection (common mode removal).
// Writes directly to OCR registers for maximum speed (no analogWrite overhead).
// ---------------------------------------------------------------
void svpwm_apply(float valpha, float vbeta, float vdc) {
  // Convert alpha-beta to three-phase using inverse Clarke
  float Va = valpha;
  float Vb = -0.5f * valpha + 0.8660254f * vbeta;
  float Vc = -0.5f * valpha - 0.8660254f * vbeta;

  // Space Vector midpoint injection (removes common-mode, maximizes utilization)
  float vmax = max(Va, max(Vb, Vc));
  float vmin = min(Va, min(Vb, Vc));
  float voff  = 0.5f * (vmax + vmin);

  Va -= voff;
  Vb -= voff;
  Vc -= voff;

  // Convert voltages to 0..255 duty cycles (centered around Vdc/2)
  // duty = (V/Vdc + 0.5) * 255
  float inv_vdc = 255.0f / vdc;
  int16_t da = (int16_t)((Va * inv_vdc) + 127.5f);
  int16_t db = (int16_t)((Vb * inv_vdc) + 127.5f);
  int16_t dc = (int16_t)((Vc * inv_vdc) + 127.5f);

  // Clamp to 0..255
  da = constrain(da, 0, 255);
  db = constrain(db, 0, 255);
  dc = constrain(dc, 0, 255);

  // Apply dead-time: high-side is reduced, low-side (inverted) naturally gets dead-time
  uint8_t ah = (uint8_t)constrain(da - DEAD_TIME, 0, 255);
  uint8_t bh = (uint8_t)constrain(db - DEAD_TIME, 0, 255);
  uint8_t ch = (uint8_t)constrain(dc - DEAD_TIME, 0, 255);

  // Write to hardware PWM registers directly (no function call overhead)
  // Timer2: AH=OCR2B (non-inv), AL=OCR2A (inverted complement)
  OCR2B = ah;
  OCR2A = ah; // Timer2 inverted mode: AL duty = (255 - ah) automatically

  // Timer0: BH=OCR0A (non-inv), BL=OCR0B (inverted complement)
  OCR0A = bh;
  OCR0B = bh; // Timer0 inverted mode: BL duty = (255 - bh) automatically

  // Timer1: CH=OCR1A (non-inv), CL=OCR1B (inverted complement)
  OCR1AL = ch;
  OCR1BL = ch; // Timer1 inverted mode: CL duty = (255 - ch) automatically
}

// ---------------------------------------------------------------
// svpwm_off()
// Forces all gate outputs LOW (safe state).
// Call during startup, mode switches, or fault conditions.
// ---------------------------------------------------------------
void svpwm_off() {
  OCR2B = 0; OCR2A = 255; // AH=0, AL=255(inverted=0)
  OCR0A = 0; OCR0B = 255; // BH=0, BL=255(inverted=0)
  OCR1AL = 0; OCR1BL = 255; // CH=0, CL=255(inverted=0)
}
