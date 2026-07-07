#pragma once
// ============================================================
// Config.h — ESP32 Motor Digital Twin, all parameters here.
// ============================================================

// --- Motor Parameters (PMSM) ---
static constexpr float MOTOR_RS         = 0.5f;     // Phase resistance (Ω)
static constexpr float MOTOR_LS         = 0.001f;   // Phase inductance (H)
static constexpr float MOTOR_KE         = 0.05f;    // Back-EMF constant (V·s/rad)
static constexpr float MOTOR_KT         = 0.05f;    // Torque constant (N·m/A)
static constexpr float MOTOR_J          = 0.0001f;  // Rotor inertia (kg·m²)
static constexpr float MOTOR_B          = 0.0001f;  // Viscous friction (N·m·s/rad)
static constexpr float MOTOR_POLE_PAIRS = 2;        // Pole pairs

// --- DC Bus ---
static constexpr float VDC              = 12.0f;    // V (matches Arduino supplyVoltage)

// --- Simulation ---
static constexpr float SIM_TS           = 0.0001f;  // 100 µs = 10 kHz
static constexpr int   TELEM_EVERY_N    = 100;      // Send telemetry every 100 steps = 100 Hz (safe for 115200 baud)

// ============================================================
// Gate Signal Input Pins (Arduino → ESP32)
// Arduino outputs 5V logic. Use a 10kΩ/20kΩ voltage divider
// to drop 5V → 3.3V before connecting to ESP32 GPIO.
// ============================================================
static constexpr int PIN_AH = 32;   // Phase A High-side
static constexpr int PIN_AL = 35;   // Phase A Low-side (Input-only, no pullups)
static constexpr int PIN_BH = 33;   // Phase B High-side
static constexpr int PIN_BL = 36;   // Phase B Low-side (Input-only, no pullups)
static constexpr int PIN_CH = 34;   // Phase C High-side (Input-only, no pullups)
static constexpr int PIN_CL = 39;   // Phase C Low-side (Input-only, no pullups)

// Set to 1 if you only wired 3 pins (AH, BH, CH) to the ESP32.
// The ESP32 will automatically assume complementary low-side gates (AL = !AH, etc.).
// Set to 0 if you wired all 6 gate pins (AH, AL, BH, BL, CH, CL).
#define USE_3PIN_BRIDGE 1

// ============================================================
// Feedback Output Pins (ESP32 → Arduino ADC)
// GPIO25 and GPIO26 have 8-bit built-in DACs — no RC filter needed.
// GPIO4 and GPIO5 use LEDC PWM — connect R=10kΩ, C=100nF RC filter.
// ============================================================
static constexpr int PIN_IA_OUT     = 25;   // DAC1 → Arduino A0
static constexpr int PIN_IB_OUT     = 26;   // DAC2 → Arduino A1
static constexpr int PIN_IC_OUT     = 4;    // LEDC PWM → RC → Arduino A2
static constexpr int PIN_THETA_OUT  = 5;    // LEDC PWM → RC → Arduino A3

// LEDC PWM settings for IC and Theta outputs
static constexpr int LEDC_FREQ      = 39062;  // ~39 kHz carrier (above RC cutoff)
static constexpr int LEDC_BITS      = 8;      // 8-bit resolution (0..255)
static constexpr int LEDC_CH_IC     = 0;
static constexpr int LEDC_CH_THETA  = 1;

// --- Current scaling ---
// Full-scale: ±CURRENT_PEAK_A maps to 0..3.3V (DAC 0..255, midpoint=127)
// Mapped so that 1.65V swing over 16.5A range equals exactly 100 mV/A sensitivity (matches Arduino currentSensitivity)
static constexpr float CURRENT_PEAK_A = 16.5f;

// --- Serial (USB to PC backend) ---
static constexpr int SERIAL_BAUD    = 115200;
