#pragma once

// ============================================================
// Config.h - Central configuration for Arduino FOC Controller
// All motor and system parameters are defined here.
// ============================================================

// --- Motor Parameters ---
#define POLE_PAIRS          2        // Number of pole pairs
#define SUPPLY_VOLTAGE      24.0f    // DC bus voltage (V)

// --- Control Loop ---
#define CONTROL_FREQ_HZ     10000    // FOC loop frequency (Hz)
#define TS                  (1.0f / CONTROL_FREQ_HZ)  // Sampling period (s)

// --- Current Sensor Pins (Analog Inputs from ESP32 via RC Filter) ---
#define PIN_IA              A0
#define PIN_IB              A1
#define PIN_IC              A2
#define PIN_THETA           A3  // Rotor electrical angle from ESP32 (0V=0, 5V=2pi)

// --- Gate Signal Output Pins ---
#define PIN_AH              3   // Timer2B
#define PIN_AL              11  // Timer2A
#define PIN_BH              6   // Timer0A
#define PIN_BL              5   // Timer0B
#define PIN_CH              9   // Timer1A
#define PIN_CL              10  // Timer1B

// --- Current Sensor Scaling (ESP32 maps -IPEAK..+IPEAK to 0..5V) ---
#define CURRENT_PEAK_A      10.0f   // Maximum peak current (A) - matches ESP32 config
#define ADC_MIDPOINT        512.0f  // ADC mid-scale (0A = 2.5V = 512 on 5V Arduino)
#define ADC_SCALE           (CURRENT_PEAK_A / ADC_MIDPOINT)  // (A per ADC count)

// --- PI Controller Limits ---
#define MAX_VD              6.0f    // Max Vd voltage output (V)
#define MAX_VQ              10.0f   // Max Vq voltage output (V)
#define MAX_VOLTAGE_VECTOR  11.0f   // Max |V| vector magnitude (V), < Vdc/sqrt(3)
#define MAX_IQ_REF          8.0f    // Max Iq reference from speed loop (A)
#define ID_REF              0.0f    // Id reference (0 for PMSM with no field weakening)

// --- Current PI Gains (tune for your motor's R and L) ---
#define KP_ID               1.5f
#define KI_ID               150.0f
#define KP_IQ               1.5f
#define KI_IQ               150.0f

// --- Speed PI Gains ---
#define KP_SPEED            0.05f
#define KI_SPEED            0.5f
#define MAX_TARGET_RPM      3000.0f
