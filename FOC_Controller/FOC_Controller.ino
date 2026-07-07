/*******************************************************************************
 * FOC_Controller.ino - Main Arduino UNO entry point
 *
 * System: Dual-MCU Hardware-in-the-Loop (HIL) Digital Twin
 * Role  : FOC Controller (Receives Ia/Ib/Ic/Theta, drives AH/AL/BH/BL/CH/CL)
 *
 * Inputs (from ESP32 via RC filter):
 *   A0 = Ia    (Phase A Current)
 *   A1 = Ib    (Phase B Current)
 *   A2 = Ic    (Phase C Current)
 *   A3 = Theta (Electrical angle 0..2pi mapped to 0..5V)
 *
 * Outputs (to ESP32 via voltage divider):
 *   D3  = AH  |  D11 = AL  (Phase A gate pair)
 *   D6  = BH  |  D5  = BL  (Phase B gate pair)
 *   D9  = CH  |  D10 = CL  (Phase C gate pair)
 *
 * FOC executes every 100 µs (10 kHz) using micros() timing in loop().
 * Serial: Type target RPM (e.g. "300") and press Enter.
 ******************************************************************************/

#include <Arduino.h>
#include "Config.h"
#include "FOC.h"
#include "SVPWM.h"

// Global FOC Controller instance
FOCController foc;

// Timing for 10 kHz control loop
static unsigned long lastControlMicros = 0;

// Loop rate counter for diagnostics
static uint32_t loopCount = 0;

// Serial monitoring
static unsigned long lastPrintMs = 0;

// ============================================================================
// setup()
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println(F("=== Arduino FOC Controller (HIL Digital Twin) ==="));
  Serial.println(F("Type target RPM and press Enter (e.g. 300)"));

  // Initialize FOC (configures PWM timers and resets integrators)
  foc.begin();

  // Set default target RPM to 300 so the motor spins automatically on boot
  foc.setTargetRPM(300.0f);

  lastControlMicros = micros();

  Serial.println(F("FOC running. Waiting for RPM command..."));
}

// ============================================================================
// loop() - 10 kHz control loop via micros() timing
// This approach avoids all timer conflicts. The Arduino UNO has only 3 timers
// (0, 1, 2) which are all consumed by the complementary PWM outputs. Using a
// 4th timer for an ISR is impossible, so micros() polling is the correct method.
// ============================================================================
void loop() {
  // --- 10 kHz FOC step (target every 100 µs, use real elapsed dt) ---
  static unsigned long maxExecMicros = 0;
  static unsigned long sumExecMicros = 0;
  static uint32_t sampleCount = 0;
  static uint32_t missedCycles = 0;

  unsigned long now = micros();
  if (now - lastControlMicros >= 100UL) {
    unsigned long start = micros();

    // If elapsed time is >= 200 µs, we missed at least one execution cycle
    if (now - lastControlMicros >= 200UL) {
      missedCycles++;
    }

    float actualDt = (float)(now - lastControlMicros) * 1e-6f;
    // Clamp dt: if something stalls for > 5ms, don't wind up integrators
    if (actualDt > 0.005f) actualDt = 0.005f;
    lastControlMicros = now;
    foc.step(actualDt);
    loopCount++;

    unsigned long execTime = micros() - start;
    if (execTime > maxExecMicros) {
      maxExecMicros = execTime;
    }
    sumExecMicros += execTime;
    sampleCount++;
  }

  // --- Serial RPM input (Non-blocking) ---
  static String inputBuffer = "";
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        float rpm = inputBuffer.toFloat();
        rpm = constrain(rpm, 0.0f, MAX_TARGET_RPM);
        foc.setTargetRPM(rpm);
        Serial.print(F("Target RPM set to: "));
        Serial.println(rpm, 1);
      }
      inputBuffer = ""; // Reset buffer
    } else {
      if (inputBuffer.length() < 16) {
        inputBuffer += c;
      } else {
        inputBuffer = ""; // Clear overflow
      }
    }
  }

  // --- Telemetry print every 127 ms ---
  // NOTE: 127ms is intentionally not a multiple of 100ms (300 RPM electrical period)
  // to avoid aliasing. At 300 RPM the electrical cycle = 100ms, so a 200ms print
  // would always catch theta at the same phase (looks "stuck"). 127ms shows true rotation.
  if (millis() - lastPrintMs >= 127UL) {
    // Compute actual loop rate (loops per 127ms → Hz)
    uint16_t rateHz = (uint16_t)((uint32_t)loopCount * 1000UL / 127UL);
    loopCount = 0;
    lastPrintMs = millis();

    float avgExec = (sampleCount > 0) ? (float)sumExecMicros / sampleCount : 0.0f;
    float cpuUsage = (avgExec / 100.0f) * 100.0f; // relative to 100 µs control period

    Serial.print(F("Rate="));    Serial.print(rateHz);
    Serial.print(F("Hz  Theta=")); Serial.print(foc.getTheta(), 3);
    Serial.print(F("rad  Id="));  Serial.print(foc.getId(), 3);
    Serial.print(F("A  Iq="));    Serial.print(foc.getIq(), 3);
    Serial.print(F("A  Avg="));   Serial.print(avgExec, 1);
    Serial.print(F("us  Max="));   Serial.print(maxExecMicros);
    Serial.print(F("us  CPU="));   Serial.print(cpuUsage, 1);
    Serial.print(F("%  Miss="));  Serial.println(missedCycles);

    // Reset statistics for next interval
    maxExecMicros = 0;
    sumExecMicros = 0;
    sampleCount = 0;
  }
}

