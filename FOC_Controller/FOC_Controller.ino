/*******************************************************************************
 * FOC_Controller.ino - Main Arduino UNO entry point
 *
 * System: Dual-MCU Hardware-in-the-Loop (HIL) Digital Twin
 * Role  : FOC Controller (Receives Ia/Ib/Ic/Theta, drives AH/AL/BH/BL/CH/CL)
 *
 * Inputs (from ESP32 via analog pins):
 *   A0 = Ia  (Phase A Current, RC-filtered PWM from ESP32)
 *   A1 = Ib  (Phase B Current, RC-filtered PWM from ESP32)
 *   A2 = Ic  (Phase C Current, RC-filtered PWM from ESP32)
 *   A3 = Theta (Electrical angle 0..2pi mapped to 0..5V from ESP32)
 *
 * Outputs (to ESP32):
 *   D3  = AH  (Phase A High-side gate)
 *   D11 = AL  (Phase A Low-side gate, complementary)
 *   D6  = BH  (Phase B High-side gate)
 *   D5  = BL  (Phase B Low-side gate, complementary)
 *   D9  = CH  (Phase C High-side gate)
 *   D10 = CL  (Phase C Low-side gate, complementary)
 *
 * The FOC algorithm runs at 10 kHz (every 100 µs) from a Timer1 ISR.
 * Serial Monitor input: type an RPM target (e.g. "300") and press Enter.
 ******************************************************************************/

#include <Arduino.h>
#include "Config.h"
#include "FOC.h"
#include "SVPWM.h"

// ----- Global FOC Controller instance -----
FOCController foc;

// ----- ISR timing flag -----
volatile bool foc_trigger = false;

// ----- Serial monitoring -----
unsigned long lastPrintMs = 0;

// ============================================================================
// Timer1 Overflow ISR - fires at exactly 10 kHz (every 100 µs)
// Sets a flag that is consumed in loop() to run the FOC step.
// We use a flag (not direct ISR execution) to avoid ADC timing issues.
// ============================================================================
ISR(TIMER1_OVF_vect) {
  foc_trigger = true;
}

// ============================================================================
// configureISRTimer()
// Reprograms Timer1 as a pure overflow timer (no PWM outputs from Timer1).
// Timer1 is used exclusively for the 10 kHz FOC interrupt trigger.
// Timer1 PWM pins (D9, D10) are handled separately by the SVPWM module
// using OCR1A/OCR1B registers AFTER this setup.
// ============================================================================
void configureISRTimer() {
  noInterrupts();
  // CTC mode: clear timer on compare match with OCR1A
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10); // CTC mode, no prescaler
  // OCR1A = 16000000 / 10000 - 1 = 1599 (triggers every 100 µs)
  OCR1A = 1599;
  TIMSK1 = _BV(OCIE1A); // Enable Timer1 Compare Match A interrupt
  interrupts();
}

// ============================================================================
// setup()
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println(F("=== Arduino FOC Controller (HIL Digital Twin) ==="));
  Serial.println(F("Type a target RPM and press Enter (e.g. 300)"));

  // Initialize FOC (sets up PWM hardware and resets integrators)
  foc.begin();

  // Start the 10 kHz ISR timer
  configureISRTimer();

  Serial.println(F("FOC running at 10 kHz. Waiting for RPM command..."));
}

// ============================================================================
// loop()
// ============================================================================
void loop() {
  // --- Handle FOC step triggered by 10 kHz ISR ---
  if (foc_trigger) {
    foc_trigger = false;
    foc.step(TS);
  }

  // --- Handle Serial RPM input ---
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      float rpm = input.toFloat();
      if (rpm < 0.0f) rpm = 0.0f;
      if (rpm > MAX_TARGET_RPM) rpm = MAX_TARGET_RPM;
      foc.setTargetRPM(rpm);
      Serial.print(F("Target RPM set to: "));
      Serial.println(rpm, 1);
    }
  }

  // --- Print telemetry every 200 ms ---
  if (millis() - lastPrintMs >= 200) {
    lastPrintMs = millis();
    Serial.print(F("Theta="));
    Serial.print(foc.getTheta(), 3);
    Serial.print(F(" rad  Id="));
    Serial.print(foc.getId(), 3);
    Serial.print(F("A  Iq="));
    Serial.print(foc.getIq(), 3);
    Serial.println(F("A"));
  }
}
