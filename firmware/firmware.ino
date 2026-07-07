#include <Arduino.h>
#include "src/main/DigitalTwin.h"

using namespace TwinProtocol;
using namespace TwinSimulation;

TwinLink twinLink(Serial, 100);
DigitalTwin twin(twinLink);

uint32_t lastLoopTime = 0;
const uint32_t TARGET_LOOP_MICROS = 1000; // 1 kHz target

// Timing Diagnostics
uint32_t maxExecTime = 0;
uint32_t missedDeadlines = 0;

void setup() {
  // Initialize communication and twin simulation
  twinLink.begin(921600);
  
  // Initialize Serial2 for Arduino #2 Bridge
  // RX2 = GPIO 16, TX2 = GPIO 17
  #if defined(ESP32)
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  #endif
  
  twin.begin();
  
  lastLoopTime = micros();
}

void loop() {
  // Parse incoming 6-byte packets from Arduino #2 Bridge
  #if defined(ESP32)
  while (Serial2.available() >= 6) {
    if (Serial2.read() == 0xAA) {
      if (Serial2.peek() == 0x55) {
        Serial2.read(); // Consume 0x55
        uint8_t dA = Serial2.read();
        uint8_t dB = Serial2.read();
        uint8_t dC = Serial2.read();
        uint8_t chk = Serial2.read();
        
        uint8_t sum = (uint8_t)(dA + dB + dC);
        if (chk == sum) {
          // Valid packet received! Inject into simulation.
          twin.setExternalDutyCycles(dA / 255.0f, dB / 255.0f, dC / 255.0f);
        }
      }
    }
  }
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
    
    // Run the main simulation engine pipeline (dt = 0.001 seconds)
    twin.step(0.001f);
    
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
