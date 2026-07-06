#pragma once

// ==============================================================================
// BLDC DIGITAL TWIN - HARDWARE CONFIGURATION
// ==============================================================================
// Edit these parameters to match the physical motor you are simulating.
// After editing, re-flash the firmware to the Arduino Uno.
// ==============================================================================

namespace TwinSimulation {
namespace Config {

// ------------------------------------------------------------------------------
// 1. Electrical Parameters
// ------------------------------------------------------------------------------
constexpr float MOTOR_RS = 0.5f;          // Phase resistance (Ohms)
constexpr float MOTOR_LS = 0.001f;        // Phase inductance (Henries)
constexpr float MOTOR_KE = 0.015f;        // Back-EMF constant (V / rad/s)
constexpr float MOTOR_POLE_PAIRS = 4.0f;  // Number of pole pairs

// ------------------------------------------------------------------------------
// 2. Mechanical Parameters
// ------------------------------------------------------------------------------
constexpr float MOTOR_INERTIA = 0.0002f;  // Rotor inertia (kg*m^2)
constexpr float MOTOR_FRICTION = 0.0001f; // Viscous friction (N*m*s)
constexpr float MOTOR_COULOMB = 0.005f;   // Coulomb friction (N*m)

// ------------------------------------------------------------------------------
// 3. Sensor Parameters
// ------------------------------------------------------------------------------
constexpr float HALL_SPACING_RAD = 6.283185f / 3.0f; // 120 electrical degrees
constexpr int ENCODER_PPR = 1024;                    // Pulses per revolution

// ------------------------------------------------------------------------------
// 4. Current Sensor / ADC Parameters
// ------------------------------------------------------------------------------
constexpr float CURRENT_SENSOR_GAIN = 0.1f;    // V/A
constexpr float CURRENT_SENSOR_OFFSET = 2.5f;  // Zero-current voltage (V)
constexpr int ADC_BITS = 10;                   // Arduino Uno is 10-bit ADC
constexpr float ADC_VREF = 5.0f;               // 5V Arduino

} // namespace Config
} // namespace TwinSimulation
