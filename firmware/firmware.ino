#include <Arduino.h>
#include "src/main/DigitalTwin.h"

using namespace TwinProtocol;
using namespace TwinSimulation;

TwinLink twinLink(Serial, 100);
DigitalTwin twin(twinLink);

uint32_t lastLoopTime = 0;
const uint32_t TARGET_LOOP_MICROS = 100; // 10 kHz target for HIL simulation

// Timing Diagnostics
uint32_t maxExecTime = 0;
uint32_t missedDeadlines = 0;

#if defined(ESP32)
// Direct Hardware PWM Reading (Inputs)
const int PIN_PWM_A = 32;
const int PIN_PWM_B = 33;
const int PIN_PWM_C = 25;

// Analog Current Outputs (PWM -> RC Filter)
const int PIN_IA_OUT = 4;
const int PIN_IB_OUT = 18;
const int PIN_IC_OUT = 19;
const int LEDC_FREQ = 39000; // 39 kHz for easy RC filtering
const int LEDC_RES = 8;      // 8-bit resolution (0-255)

// Digital Encoder Outputs
const int PIN_ENC_A = 21;
const int PIN_ENC_B = 22;
const int PIN_ENC_Z = 23;

// Variables for ultra-high-resolution duty cycle tracking
volatile uint32_t highTimeA = 0;
volatile uint32_t periodA = 30720; // Default to 128us period at 240MHz
volatile uint32_t lastRiseA = 0;

volatile uint32_t highTimeB = 0;
volatile uint32_t periodB = 30720;
volatile uint32_t lastRiseB = 0;

volatile uint32_t highTimeC = 0;
volatile uint32_t periodC = 30720;
volatile uint32_t lastRiseC = 0;

void IRAM_ATTR isrA() {
  uint32_t now = xthal_get_ccount();
  if (digitalRead(PIN_PWM_A)) { // Rising edge
    periodA = now - lastRiseA;
    lastRiseA = now;
  } else { // Falling edge
    highTimeA = now - lastRiseA;
  }
}

void IRAM_ATTR isrB() {
  uint32_t now = xthal_get_ccount();
  if (digitalRead(PIN_PWM_B)) {
    periodB = now - lastRiseB;
    lastRiseB = now;
  } else {
    highTimeB = now - lastRiseB;
  }
}

void IRAM_ATTR isrC() {
  uint32_t now = xthal_get_ccount();
  if (digitalRead(PIN_PWM_C)) {
    periodC = now - lastRiseC;
    lastRiseC = now;
  } else {
    highTimeC = now - lastRiseC;
  }
}
#endif

void setup() {
  // Initialize communication and twin simulation
  twinLink.begin(921600);
  
  #if defined(ESP32)
  pinMode(PIN_PWM_A, INPUT);
  pinMode(PIN_PWM_B, INPUT);
  pinMode(PIN_PWM_C, INPUT);
  
  pinMode(PIN_ENC_A, OUTPUT);
  pinMode(PIN_ENC_B, OUTPUT);
  pinMode(PIN_ENC_Z, OUTPUT);
  
  // Setup PWM Channels for Current Outputs (ESP32 v3 API)
  ledcAttach(PIN_IA_OUT, LEDC_FREQ, LEDC_RES);
  ledcAttach(PIN_IB_OUT, LEDC_FREQ, LEDC_RES);
  ledcAttach(PIN_IC_OUT, LEDC_FREQ, LEDC_RES);
  
  attachInterrupt(digitalPinToInterrupt(PIN_PWM_A), isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_PWM_B), isrB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_PWM_C), isrC, CHANGE);
  #endif
  
  twin.begin();
  
  lastLoopTime = micros();
}

void loop() {
  #if defined(ESP32)
  uint32_t now = xthal_get_ccount();
  float dA, dB, dC;

  // If no edges for ~2 periods (61440 cycles), the pin is stuck at 0% or 100%
  if (now - lastRiseA > 61440) {
    dA = digitalRead(PIN_PWM_A) ? 1.0f : 0.0f;
  } else {
    dA = (float)highTimeA / (float)periodA;
  }
  
  if (now - lastRiseB > 61440) {
    dB = digitalRead(PIN_PWM_B) ? 1.0f : 0.0f;
  } else {
    dB = (float)highTimeB / (float)periodB;
  }
  
  if (now - lastRiseC > 61440) {
    dC = digitalRead(PIN_PWM_C) ? 1.0f : 0.0f;
  } else {
    dC = (float)highTimeC / (float)periodC;
  }

  // Constrain sanity check
  if (dA < 0.0f) dA = 0.0f; if (dA > 1.0f) dA = 1.0f;
  if (dB < 0.0f) dB = 0.0f; if (dB > 1.0f) dB = 1.0f;
  if (dC < 0.0f) dC = 0.0f; if (dC > 1.0f) dC = 1.0f;

  // Inject into simulation
  twin.setExternalDutyCycles(dA, dB, dC);
  #endif

  uint32_t currentMicros = micros();
  uint32_t elapsedMicros = currentMicros - lastLoopTime;
  
  // Deterministic 1 kHz Loop Scheduling
  if (elapsedMicros >= TARGET_LOOP_MICROS) {
    lastLoopTime = currentMicros;
    
    // Detect jitter/overruns before execution
    if (elapsedMicros > TARGET_LOOP_MICROS + 50) {
      missedDeadlines++;
    }
    
    // Track execution time
    uint32_t startExec = micros();
    
    // Inject diagnostics for telemetry transmission
    twin.setDiagnostics(maxExecTime, missedDeadlines);
    
    // Run the main simulation engine pipeline (dt = 0.0001 seconds for 10kHz)
    twin.step(0.0001f);
    
    #if defined(ESP32)
    // --- 1. OUTPUT ANALOG CURRENTS (PWM for RC Filter) ---
    const TwinSimulation::ElectricalState& elecState = twin.getElectricalState();
    
    // Map -10A to +10A to 0-3.3V (0-255 PWM duty)
    // 0A = 1.65V = 127 duty
    float ia_duty = (elecState.ia / 10.0f) * 127.0f + 127.0f;
    float ib_duty = (elecState.ib / 10.0f) * 127.0f + 127.0f;
    float ic_duty = (elecState.ic / 10.0f) * 127.0f + 127.0f;
    
    ledcWrite(PIN_IA_OUT, (uint32_t)constrain(ia_duty, 0.0f, 255.0f));
    ledcWrite(PIN_IB_OUT, (uint32_t)constrain(ib_duty, 0.0f, 255.0f));
    ledcWrite(PIN_IC_OUT, (uint32_t)constrain(ic_duty, 0.0f, 255.0f));

    // --- 2. OUTPUT ENCODER PULSES ---
    const TwinSimulation::EncoderState& encState = twin.getEncoderState();
    digitalWrite(PIN_ENC_A, encState.a);
    digitalWrite(PIN_ENC_B, encState.b);
    digitalWrite(PIN_ENC_Z, encState.index);
    #endif
    
    uint32_t endExec = micros();
    uint32_t execTime = endExec - startExec;
    
    // Diagnostics recording
    if (execTime > maxExecTime) {
      maxExecTime = execTime;
    }
    
    // Detect if execution time exceeded the 1ms budget
    if (execTime >= TARGET_LOOP_MICROS) {
      missedDeadlines++;
    }
  }
}
