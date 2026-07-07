#pragma once
#include <Arduino.h>

struct MotorState {
  // Phase-to-neutral voltages (V)
  float va;
  float vb;
  float vc;

  // Phase currents (A)
  float ia;
  float ib;
  float ic;

  // Mechanical states
  float torqueElec;           // Electromagnetic torque (N·m)
  float mechSpeedRad;         // Mechanical speed (rad/s)
  float mechSpeedRPM;         // Mechanical speed (RPM)
  float mechAngle;            // Mechanical angle (rad, 0..2π)
  float elecAngle;            // Electrical angle (rad, 0..2π)

  // Thermal state
  float temperature;          // Estimated motor temperature (°C)

  // Gate inputs (from controller)
  bool ah, al;
  bool bh, bl;
  bool ch, cl;

  // Sensor states
  uint8_t hallA;
  uint8_t hallB;
  uint8_t hallC;
  uint16_t encoderRawAngle;   // 12-bit emulated AS5600 register value (0..4095)

  // Diagnostics
  uint32_t faultFlags;
};
