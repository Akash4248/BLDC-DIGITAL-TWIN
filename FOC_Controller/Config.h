#pragma once
// ============================================================
// Config.h - Arduino UNO FOC Controller Configuration
// ============================================================

// --- Motor Parameters ---
#define POLE_PAIRS          2
#define SUPPLY_VOLTAGE      12.0f    // DC bus voltage (V) (matches ESP32 VDC)

// --- Control Loop ---
#define CONTROL_PERIOD_US   100UL    // 100 µs = 10 kHz

// Fixed timestep for PI integrators
#define TS                  0.0001f

// --- Gate Output Pins ---
// Timer2 → Phase A:  D3 = AH (OC2B, non-inv),  D11 = AL (OC2A, inverted)
// Timer0 → Phase B:  D6 = BH (OC0A, non-inv),  D5  = BL (OC0B, inverted)
// Timer1 → Phase C:  D9 = CH (OC1A, non-inv),  D10 = CL (OC1B, inverted)
#define PIN_AH  3
#define PIN_AL  11
#define PIN_BH  6
#define PIN_BL  5
#define PIN_CH  9
#define PIN_CL  10

// --- Feedback Input Pins ---
#define PIN_IA      A0    // Phase A current (from ESP32 RC filter)
#define PIN_IB      A1    // Phase B current (from ESP32 RC filter)
#define PIN_IC      A2    // Phase C current (from ESP32 RC filter)
#define PIN_THETA   A3    // Electrical angle 0..2π → 0..5V (from ESP32)

// --- Current Sensor Scaling ---
// ESP32 maps ±CURRENT_PEAK_A to 0..3.3V via RC-filtered PWM
// On 5V Arduino: 3.3V reads as ~675.18 ADC, 0A midpoint ≈ 337.59 ADC
#define CURRENT_PEAK_A      16.5f    // Matches ESP32 scale
#define ADC_MIDPOINT        337.59f
#define ADC_SCALE           (CURRENT_PEAK_A / 337.59f)

// --- PI Limits ---
#define MAX_VD              5.0f
#define MAX_VQ              11.0f
#define MAX_VOLTAGE_VECTOR  11.0f
#define MAX_IQ_REF          6.0f
#define ID_REF              0.0f

// --- Current PI Gains ---
#define KP_ID               1.5f
#define KI_ID               100.0f
#define KP_IQ               1.5f
#define KI_IQ               100.0f

// --- Speed PI Gains ---
#define KP_SPEED            0.05f
#define KI_SPEED            0.5f
#define MAX_TARGET_RPM      3000.0f

// ============================================================
// OPEN LOOP TEST MODE
// Set to 1 to verify SVPWM sine waves WITHOUT the ESP32 connected.
// Theta is generated internally from target RPM.
// Set to 0 for closed-loop HIL operation.
// ============================================================
#define OPEN_LOOP_TEST      0

// Fixed Vq amplitude for open-loop test (V). Use ~30% of SUPPLY_VOLTAGE.
#define OPEN_LOOP_VQ        4.0f
