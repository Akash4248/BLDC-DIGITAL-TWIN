/*******************************************************************************
 * firmware.ino - ESP32 Motor Digital Twin (HIL)
 *
 * System: Dual-MCU Hardware-in-the-Loop (HIL) Digital Twin
 * Role  : Motor Digital Twin (Receives gate signals from Arduino, runs
 *         continuous PMSM simulation at 10 kHz, and outputs analog feedback)
 *
 * Architecture Layers:
 *   1. Gate Reading: Inverter::readGates() reads AH, BH, CH pins
 *   2. Decode Inverter: Inverter::decode() computes Va, Vb, Vc
 *   3. Motor Model: MotorModel::step() runs Forward Euler PMSM state space equations
 *   4. Feedback Output: CurrentOutput::writeCurrent/writeTheta writes currents/theta
 *
 * Timer ISR triggers every 100 µs (10 kHz) via esp_timer.
 ******************************************************************************/

#include <Arduino.h>
#include <esp_timer.h>
#include <Wire.h>
#include <Arduino.h>
#include <Wire.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "src/Config.h"
#include "src/SimulationEngine.h"
#include "src/TelemetryManager.h"

#define AS5600_SLAVE_ADDR 0x36

// --- Global Digital Twin and Telemetry Manager Instances ---
static SimulationEngine  engine;
static TelemetryManager telemetry;

// --- Timing and State Variables ---
static volatile bool simTrigger = false;
static uint32_t stepCounter = 0;

// --- AS5600 Emulation State ---
volatile uint8_t i2cRegisterPointer = 0;
volatile uint16_t latestRawAngle = 0;

// --- I2C Slave Callbacks for AS5600 Emulation ---
void onI2CReceive(int numBytes) {
  if (numBytes > 0) {
    i2cRegisterPointer = Wire.read();
    // Flush remaining bytes if any
    while (Wire.available()) {
      Wire.read();
    }
  }
}

void onI2CRequest() {
  // Pack the 12-bit mechanical angle (0..4095) in MSB-first format
  uint8_t responseData[2];
  responseData[0] = (latestRawAngle >> 8) & 0x0F;
  responseData[1] = latestRawAngle & 0xFF;
  Wire.write(responseData, 2);
}

// ============================================================================
// Timer ISR callback - Fires every 100 µs (10 kHz)
// Runs in the ESP32 timer task context.
// ============================================================================
static void IRAM_ATTR onSimTimer(void* arg) {
  simTrigger = true;
}

// ============================================================================
// setup()
// ============================================================================
void setup() {
  // Disable brownout detector to bypass startup voltage transient dips
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Initialize Serial for high-speed binary telemetry
  Serial.begin(SERIAL_BAUD);

  // Initialize I2C Slave at AS5600 address (0x36) on default SDA/SCL pins (GPIO 21 / 22)
  Wire.begin(AS5600_SLAVE_ADDR);
  Wire.onReceive(onI2CReceive);
  Wire.onRequest(onI2CRequest);

  // Initialize unified simulation engine
  engine.begin();

  // Setup periodic simulation timer at 100 µs intervals (10 kHz)
  esp_timer_handle_t timerHandle;
  esp_timer_create_args_t timerArgs = {
    .callback        = onSimTimer,
    .arg             = nullptr,
    .dispatch_method = ESP_TIMER_TASK,
    .name            = "simulation_timer",
    .skip_unhandled_events = true
  };
  esp_timer_create(&timerArgs, &timerHandle);
  esp_timer_start_periodic(timerHandle, 100); // 100 µs

  // Print startup notification (note: may corrupt the very first binary package
  // but helpful for developer debugging)
  delay(100);
  Serial.println("\n--- ESP32 Motor Digital Twin Core Initialized at 10 kHz ---");
}

// ============================================================================
// loop()
// ============================================================================
void loop() {
  if (simTrigger) {
    simTrigger = false;

    // Run one step of the coordinated simulation engine
    engine.step();

    // Send high-speed binary telemetry at configured interval (100 Hz)
    stepCounter++;
    if (stepCounter >= TELEM_EVERY_N) {
      stepCounter = 0;
      telemetry.send(engine.getState());
    }
  }
}
