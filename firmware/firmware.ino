/*******************************************************************************
 * firmware.ino - ESP32 Motor Digital Twin entry point
 *
 * System: Dual-MCU Hardware-in-the-Loop (HIL) Digital Twin
 * Role  : Motor Digital Twin (Receives AH/AL/BH/BL/CH/CL, outputs Ia/Ib/Ic/Theta)
 *
 * Inputs (from Arduino via voltage divider 5V->3.3V):
 *   GPIO 32 = AH  (Phase A High-side gate)
 *   GPIO 14 = AL  (Phase A Low-side gate)
 *   GPIO 33 = BH  (Phase B High-side gate)
 *   GPIO 27 = BL  (Phase B Low-side gate)
 *   GPIO 25 = CH  (Phase C High-side gate)
 *   GPIO 26 = CL  (Phase C Low-side gate)
 *
 * Outputs (to Arduino via RC low-pass filter):
 *   GPIO 4  = Ia    (Phase A Current, PWM+RC -> analog)
 *   GPIO 18 = Ib    (Phase B Current, PWM+RC -> analog)
 *   GPIO 19 = Ic    (Phase C Current, PWM+RC -> analog)
 *   GPIO 21 = Theta (Electrical angle 0..2pi, PWM+RC -> 0..3.3V)
 *
 * USB Serial: Sends telemetry to PC backend (BLDC Digital Twin Dashboard)
 *
 * Simulation runs at 10 kHz (100 µs per step) using esp_timer ISR.
 ******************************************************************************/

#include <Arduino.h>
#include <esp_timer.h>
#include "src/Config.h"
#include "src/Inverter.h"
#include "src/MotorModel.h"
#include "src/Encoder.h"
#include "src/CurrentOutput.h"

// ----- Module Instances -----
static Inverter          inverter;
static MotorModel        motor;
static GateSignalReader  gateReader;
static CurrentOutput     currentOut;

// ----- Timing -----
static volatile bool simTrigger = false;
static uint32_t telemTimer = 0;

// ----- Telemetry Packet (binary, for PC backend) -----
#pragma pack(push, 1)
struct TelemetryPacket {
  uint16_t magic;        // 0xAA55
  uint8_t  type;         // 0x01
  uint8_t  length;       // payload bytes
  uint16_t seq;
  uint32_t unused;
  float    ia, ib, ic;
  float    rotor_angle;
  float    rotor_speed;
};
#pragma pack(pop)

static uint16_t seqNum = 0;

// ----- esp_timer ISR: fires every 100 µs (10 kHz) -----
static void IRAM_ATTR onSimTimer(void* arg) {
  simTrigger = true;
}

// ============================================================================
// setup()
// ============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD);

  // Initialize gate signal reader (attaches interrupts on AH, BH, CH)
  gateReader.begin();

  // Initialize analog feedback outputs (LEDC PWM)
  currentOut.begin();

  // Create a repeating timer at 100 µs (10 kHz)
  esp_timer_handle_t simTimerHandle;
  esp_timer_create_args_t timerArgs = {
    .callback        = onSimTimer,
    .arg             = nullptr,
    .dispatch_method = ESP_TIMER_TASK,
    .name            = "sim_timer",
    .skip_unhandled_events = true
  };
  esp_timer_create(&timerArgs, &simTimerHandle);
  esp_timer_start_periodic(simTimerHandle, 100); // 100 µs = 10 kHz

  Serial.println("ESP32 Motor Digital Twin started at 10 kHz");
}

// ============================================================================
// loop()
// ============================================================================
void loop() {
  if (!simTrigger) return;
  simTrigger = false;

  // --- Step 1: Read gate signals -> compute duty cycles ---
  float dutyA = gateReader.getDutyA();
  float dutyB = gateReader.getDutyB();
  float dutyC = gateReader.getDutyC();

  // --- Step 2: Inverter model -> compute phase voltages ---
  inverter.update(dutyA, dutyB, dutyC, VDC);
  const InverterState& inv = inverter.getState();

  // --- Step 3: Motor model -> advance simulation by one step ---
  motor.step(inv.va, inv.vb, inv.vc, SIM_TS);
  const MotorState& st = motor.getState();

  // --- Step 4: Output Ia, Ib, Ic, Theta back to Arduino ---
  currentOut.writeCurrent(st.ia, st.ib, st.ic);
  currentOut.writeTheta(st.elecAngle);

  // --- Step 5: Send telemetry to PC dashboard (every 1 ms) ---
  telemTimer++;
  if (telemTimer >= (TELEM_INTERVAL_US / (uint32_t)(SIM_TS * 1000000.0f))) {
    telemTimer = 0;

    TelemetryPacket pkt;
    pkt.magic       = 0xAA55;
    pkt.type        = 0x01;
    pkt.length      = sizeof(TelemetryPacket) - 4; // exclude magic+type+length
    pkt.seq         = seqNum++;
    pkt.unused      = 0;
    pkt.ia          = st.ia;
    pkt.ib          = st.ib;
    pkt.ic          = st.ic;
    pkt.rotor_angle = st.elecAngle;
    pkt.rotor_speed = st.rotorSpeedRPM;

    Serial.write((const uint8_t*)&pkt, sizeof(pkt));
  }
}
