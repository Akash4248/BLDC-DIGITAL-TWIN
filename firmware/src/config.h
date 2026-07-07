#pragma once

// ============================================================
// Config.h - ESP32 Motor Digital Twin Configuration
// All motor and simulation parameters are defined here.
// ============================================================

// --- Motor Parameters (3-phase PMSM/BLDC) ---
static constexpr float MOTOR_RS        = 0.5f;     // Phase resistance (Ohm)
static constexpr float MOTOR_LS        = 0.001f;   // Phase inductance (H)
static constexpr float MOTOR_KE        = 0.05f;    // Back-EMF constant (V/(rad/s))
static constexpr float MOTOR_KT        = 0.05f;    // Torque constant (N.m/A) = KE for SI PMSM
static constexpr float MOTOR_J         = 0.0001f;  // Rotor inertia (kg.m^2)
static constexpr float MOTOR_B         = 0.0001f;  // Viscous friction (N.m.s/rad)
static constexpr float MOTOR_POLE_PAIRS= 2;        // Number of pole pairs

// --- System Parameters ---
static constexpr float VDC             = 24.0f;    // DC Bus Voltage (V)
static constexpr float SIM_TS          = 0.0001f;  // Simulation step (s) = 10 kHz

// --- Gate Signal Input Pins (from Arduino, 5V -> 3.3V voltage divider required!) ---
static constexpr int PIN_AH = 32;
static constexpr int PIN_AL = 14;
static constexpr int PIN_BH = 33;
static constexpr int PIN_BL = 27;
static constexpr int PIN_CH = 25;
static constexpr int PIN_CL = 26;

// --- Analog Output Pins (PWM + RC Filter -> smooth analog to Arduino) ---
// Current outputs: mapped -IPEAK..+IPEAK to 0..3.3V
static constexpr int PIN_IA_OUT    = 4;
static constexpr int PIN_IB_OUT    = 18;
static constexpr int PIN_IC_OUT    = 19;
// Theta output: mapped 0..2pi to 0..3.3V
static constexpr int PIN_THETA_OUT = 21;

// --- Current Output Scaling ---
static constexpr float CURRENT_PEAK_A = 10.0f;   // Maximum current magnitude (A) for DAC scaling

// --- PWM (for analog output simulation) ---
static constexpr int LEDC_FREQ         = 39062;  // ~39 kHz (above RC filter cutoff)
static constexpr int LEDC_RESOLUTION   = 8;      // 8-bit (0..255)
static constexpr int LEDC_CH_IA        = 0;
static constexpr int LEDC_CH_IB        = 1;
static constexpr int LEDC_CH_IC        = 2;
static constexpr int LEDC_CH_THETA     = 3;

// --- USB Serial (to PC backend) ---
static constexpr int SERIAL_BAUD      = 921600;

// --- Telemetry ---
static constexpr int TELEM_INTERVAL_US = 1000;   // Send telemetry every 1 ms (1 kHz)
