// SVPWM.h - Space Vector PWM with complementary gate signal output
#pragma once
#include <Arduino.h>
#include "Config.h"

// Initializes all 6 PWM output pins and synchronizes hardware timers.
// This MUST be called once in setup() before any gate signals are driven.
void svpwm_init();

// Applies Space Vector Modulation given (valpha, vbeta) and Vdc.
// Computes the three-phase duty cycles and writes them directly to the
// hardware PWM registers as 6 complementary gate signals (AH/AL, BH/BL, CH/CL).
// Includes a small fixed dead-time offset to prevent shoot-through.
void svpwm_apply(float valpha, float vbeta, float vdc);

// Sets all gate outputs LOW (safe state). Call during mode transitions.
void svpwm_off();
