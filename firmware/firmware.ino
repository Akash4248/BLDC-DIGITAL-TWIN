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
  twinLink.begin(115200);
  twin.begin();
  
  lastLoopTime = micros();
}

void loop() {
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
