// Current.h - Phase current ADC reading
#pragma once
#include <Arduino.h>
#include "Config.h"

struct PhaseCurrents {
  float ia, ib, ic;
};

// Initializes and calibrates current sensor ADC offsets.
void initCurrentSensors();

// Reads and scales the three phase currents from the ESP32 analog feedback.
// The ESP32 maps -CURRENT_PEAK_A..+CURRENT_PEAK_A to 0V..5V (ADC 0..1023).
PhaseCurrents readPhaseCurrents();
