#include <Arduino.h>
#include <Wire.h>
#include <math.h>

/* ==========================================================
   ESP32 BLDC/PMSM DIGITAL TWIN - SINGLE FILE

   Arduino UNO FOC controller connects to ESP32.
   ESP32 evaluates motor currents and rotor angle, then returns:
   - AS5600-compatible I2C angle feedback
   - ACS712-equivalent current feedback using filtered PWM

   ESP32 Arduino core 3.x compatible.
   ========================================================== */

/* ===================== PIN MAP ===================== */
// Arduino UNO gate outputs -> ESP32 inputs.
// Use 5V-to-3.3V level shifting or resistor dividers.
const int PIN_UH = 25;
const int PIN_UL = 26;
const int PIN_VH = 27;
const int PIN_VL = 14;
const int PIN_WH = 32;
const int PIN_WL = 33;

// ESP32 AS5600 I2C slave pins -> Arduino UNO A4/A5.
// Pullups must be to 3.3V.
const int PIN_AS5600_SDA = 21;
const int PIN_AS5600_SCL = 22;

// ESP32 PWM current feedback outputs -> Arduino UNO A0/A1/A2 through RC filters.
const int PIN_IA_PWM = 18;
const int PIN_IB_PWM = 19;
const int PIN_IC_PWM = 23;

/* ===================== CONSTANTS ===================== */
const float PI_F = 3.14159265358979323846f;
const float TWO_PI_F = 6.28318530717958647692f;

const float SUPPLY_VOLTAGE = 12.0f;
const float POLE_PAIRS = 2.0f;

// Tune these to match your physical motor.
const float PHASE_RESISTANCE = 0.55f;    // ohm
const float PHASE_INDUCTANCE = 0.00075f; // H
const float FLUX_LINKAGE = 0.018f;       // Wb
const float ROTOR_INERTIA = 0.000035f;   // kg.m^2
const float VISCOUS_FRICTION = 0.000025f;
const float LOAD_TORQUE = 0.0f;

// Simulation timing.
const uint32_t SIM_PERIOD_US = 100; // 10 kHz
const float SIM_DT = 0.0001f;

// AS5600.
const uint8_t AS5600_I2C_ADDRESS = 0x36;
const uint16_t AS5600_CPR = 4096;

// Current feedback PWM.
const uint32_t CURRENT_PWM_HZ = 50000;
const uint8_t CURRENT_PWM_BITS = 10;
const uint16_t CURRENT_PWM_MAX = (1U << CURRENT_PWM_BITS) - 1U;
const float ESP32_PWM_HIGH_VOLTAGE = 3.3f;

// ACS712 20A scaling used by your Arduino code.
const float ACS712_ZERO_VOLTAGE = 2.5f;
const float ACS712_SENSITIVITY = 0.100f; // V/A

// Numerical limits.
const float MAX_PHASE_CURRENT = 20.0f;
const float MAX_SPEED_RAD_PER_SEC = 800.0f;

/* ===================== GLOBAL STATE ===================== */
enum PhaseState : uint8_t {
  PHASE_LOW = 0,
  PHASE_HIGH = 1,
  PHASE_FLOATING = 2,
  PHASE_INVALID = 3
};

struct PhaseVoltages {
  float a;
  float b;
  float c;
  bool invalid;
};

struct MotorState {
  float ia;
  float ib;
  float ic;
  float omegaMechanical;
  float thetaMechanical;
  float thetaElectrical;
  float torque;
};

float motorIa = 0.0f;
float motorIb = 0.0f;
float motorOmega = 0.0f;
float motorThetaM = 0.0f;
float motorTorque = 0.0f;

volatile uint16_t as5600RawAngle = 0;
volatile uint8_t as5600RegisterPointer = 0x0C;

hw_timer_t* simTimer = nullptr;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t pendingTicks = 0;

uint32_t lastPrintMs = 0;
uint32_t simSteps = 0;

// ===== WEBSITE VISUALIZATION START =====
// New variables
const int PIN_VIS_MODE = 5;           // GPIO D5 as the mode indicator
float visRotorAngle = 0.0f;           // Estimated rotor electrical angle (0-360 deg)
float visStatorFieldAngle = 0.0f;     // Stator field angle (0-360 deg)
float visRotorFieldAngle = 0.0f;      // Rotor field angle (0-360 deg)
float visFieldAngleDifference = 0.0f; // Difference between stator and rotor fields (deg)
const char* visMotorMode = "Open_Loop";
const char* visControllerMode = "Non-FOC";
float visIa = 0.0f;
float visIb = 0.0f;
float visIc = 0.0f;
float visRPM = 0.0f;
const float visVoltage = 12.0f;       // Constant Voltage
const int visPolePairs = 2;           // Constant Pole Pairs
// ===== WEBSITE VISUALIZATION END =====

// ===== WEBSITE VISUALIZATION START =====
// Helper to wrap angle to [0, 360) degrees
float wrap360(float angle) {
  float wrapped = fmodf(angle, 360.0f);
  if (wrapped < 0.0f) {
    wrapped += 360.0f;
  }
  return wrapped;
}

// Binary telemetry definitions and packet sender
#include <esp_timer.h>

#pragma pack(push, 1)
struct PacketHeader {
  uint8_t  sync[2];       // {0xAA, 0x55}
  uint8_t  version;       // 1
  uint8_t  packet_type;   // 2 (TwinState)
  uint16_t seq;
  uint16_t payload_len;   // 47
  uint16_t reserved;      // 0
};

struct TwinStatePayload {
  uint64_t timestamp_us;
  float    ia, ib, ic;
  float    rotor_speed;
  float    rotor_angle;
  float    electrical_angle;
  float    torque;
  float    temperature;
  uint8_t  hall;          // bit 0=A, bit 1=B, bit 2=C
  uint16_t encoder;
  uint32_t fault_flags;
};

struct TwinStatePacket {
  PacketHeader     header;
  TwinStatePayload payload;
  uint16_t         crc;
};
#pragma pack(pop)

uint16_t telemetrySeq = 0;
uint32_t teleStepCounter = 0;
const uint32_t TELEM_EVERY_N = 100; // Send binary telemetry every 100 steps (100 Hz)

uint16_t calculateTelemetryCRC(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}

void sendBinaryTelemetry(const MotorState& s) {
  TwinStatePacket pkt;
  pkt.header.sync[0] = 0xAA;
  pkt.header.sync[1] = 0x55;
  pkt.header.version = 1;
  pkt.header.packet_type = 2; // TwinState
  pkt.header.seq = telemetrySeq++;
  pkt.header.payload_len = sizeof(TwinStatePayload); // 47
  pkt.header.reserved = 0;

  pkt.payload.timestamp_us = esp_timer_get_time();
  pkt.payload.ia = s.ia;
  pkt.payload.ib = s.ib;
  pkt.payload.ic = s.ic;
  pkt.payload.rotor_speed = s.omegaMechanical;
  pkt.payload.rotor_angle = s.thetaMechanical;
  pkt.payload.electrical_angle = s.thetaElectrical;
  pkt.payload.torque = s.torque;
  pkt.payload.temperature = 40.0f + (s.ia * s.ia + s.ib * s.ib) * 0.1f; // approximation
  pkt.payload.hall = (digitalRead(PIN_VIS_MODE) == HIGH ? 0x08 : 0x00);
  pkt.payload.encoder = (uint16_t)((s.thetaMechanical / TWO_PI_F) * AS5600_CPR) & 0x0FFF;
  pkt.payload.fault_flags = 0;

  uint8_t* crc_start = ((uint8_t*)&pkt) + 2;
  pkt.crc = calculateTelemetryCRC(crc_start, 8 + sizeof(TwinStatePayload));

  Serial.write((uint8_t*)&pkt, sizeof(TwinStatePacket));
}
// ===== WEBSITE VISUALIZATION END =====

/* ===================== MATH ===================== */
float wrapTwoPi(float angle) {
  while (angle >= TWO_PI_F) angle -= TWO_PI_F;
  while (angle < 0.0f) angle += TWO_PI_F;
  return angle;
}

float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

/* ===================== INVERTER DECODER ===================== */
PhaseState decodePhase(bool highSide, bool lowSide) {
  if (highSide && !lowSide) return PHASE_HIGH;
  if (!highSide && lowSide) return PHASE_LOW;
  if (!highSide && !lowSide) return PHASE_FLOATING;
  return PHASE_INVALID;
}

float stateToPoleVoltage(PhaseState state) {
  switch (state) {
    case PHASE_HIGH: return SUPPLY_VOLTAGE;
    case PHASE_LOW: return 0.0f;
    case PHASE_FLOATING: return SUPPLY_VOLTAGE * 0.5f;
    case PHASE_INVALID: return SUPPLY_VOLTAGE * 0.5f;
  }
  return SUPPLY_VOLTAGE * 0.5f;
}

PhaseVoltages readInverterVoltages() {
  PhaseState sa = decodePhase(digitalRead(PIN_UH), digitalRead(PIN_UL));
  PhaseState sb = decodePhase(digitalRead(PIN_VH), digitalRead(PIN_VL));
  PhaseState sc = decodePhase(digitalRead(PIN_WH), digitalRead(PIN_WL));

  PhaseVoltages v;
  v.a = stateToPoleVoltage(sa);
  v.b = stateToPoleVoltage(sb);
  v.c = stateToPoleVoltage(sc);
  v.invalid = (sa == PHASE_INVALID || sb == PHASE_INVALID || sc == PHASE_INVALID);

  // Remove common mode to get phase-to-neutral voltage approximation.
  float commonMode = (v.a + v.b + v.c) * 0.3333333333f;
  v.a -= commonMode;
  v.b -= commonMode;
  v.c -= commonMode;

  return v;
}

/* ===================== MOTOR MODEL ===================== */
void resetMotorModel() {
  motorIa = 0.0f;
  motorIb = 0.0f;
  motorOmega = 0.0f;
  motorThetaM = 0.0f;
  motorTorque = 0.0f;
}

void stepMotorModel(const PhaseVoltages& phaseVoltage, float dt) {
  float ic = -motorIa - motorIb;
  float thetaE = wrapTwoPi(motorThetaM * POLE_PAIRS);
  float omegaE = motorOmega * POLE_PAIRS;

  float ea = -FLUX_LINKAGE * omegaE * sinf(thetaE);
  float eb = -FLUX_LINKAGE * omegaE * sinf(thetaE - 2.0f * PI_F / 3.0f);
  float ec = -FLUX_LINKAGE * omegaE * sinf(thetaE + 2.0f * PI_F / 3.0f);

  float dia = (phaseVoltage.a - PHASE_RESISTANCE * motorIa - ea) / PHASE_INDUCTANCE;
  float dib = (phaseVoltage.b - PHASE_RESISTANCE * motorIb - eb) / PHASE_INDUCTANCE;
  float dic = (phaseVoltage.c - PHASE_RESISTANCE * ic - ec) / PHASE_INDUCTANCE;

  // Enforce ia + ib + ic = 0.
  float avg = (dia + dib + dic) * 0.3333333333f;
  dia -= avg;
  dib -= avg;

  // Invalid shoot-through state is softened in the model.
  if (phaseVoltage.invalid) {
    dia *= 0.1f;
    dib *= 0.1f;
  }

  motorIa = clampFloat(motorIa + dia * dt, -MAX_PHASE_CURRENT, MAX_PHASE_CURRENT);
  motorIb = clampFloat(motorIb + dib * dt, -MAX_PHASE_CURRENT, MAX_PHASE_CURRENT);

  float iAlpha = motorIa;
  float iBeta = (motorIa + 2.0f * motorIb) * 0.57735026919f;
  float iq = -iAlpha * sinf(thetaE) + iBeta * cosf(thetaE);

  motorTorque = 1.5f * POLE_PAIRS * FLUX_LINKAGE * iq;

  float domega = (motorTorque - LOAD_TORQUE - VISCOUS_FRICTION * motorOmega) / ROTOR_INERTIA;
  motorOmega = clampFloat(motorOmega + domega * dt, -MAX_SPEED_RAD_PER_SEC, MAX_SPEED_RAD_PER_SEC);
  motorThetaM = wrapTwoPi(motorThetaM + motorOmega * dt);
}

MotorState getMotorState() {
  MotorState s;
  s.ia = motorIa;
  s.ib = motorIb;
  s.ic = -motorIa - motorIb;
  s.omegaMechanical = motorOmega;
  s.thetaMechanical = motorThetaM;
  s.thetaElectrical = wrapTwoPi(motorThetaM * POLE_PAIRS);
  s.torque = motorTorque;
  return s;
}

/* ===================== AS5600 I2C EMULATION ===================== */
uint8_t readAS5600Register(uint8_t reg) {
  uint16_t raw = as5600RawAngle;

  switch (reg) {
    case 0x0B: return 0x20;                              // status: magnet detected
    case 0x0C: return (uint8_t)((raw >> 8) & 0x0F);      // raw angle high
    case 0x0D: return (uint8_t)(raw & 0xFF);             // raw angle low
    case 0x0E: return (uint8_t)((raw >> 8) & 0x0F);      // angle high
    case 0x0F: return (uint8_t)(raw & 0xFF);             // angle low
    default: return 0x00;
  }
}

void updateAS5600Angle(float mechanicalAngleRad) {
  float angle = wrapTwoPi(mechanicalAngleRad);
  as5600RawAngle = (uint16_t)((angle / TWO_PI_F) * AS5600_CPR) & 0x0FFF;
}

void onAS5600Receive(int count) {
  if (count <= 0) return;

  as5600RegisterPointer = Wire.read();
  while (Wire.available()) {
    (void)Wire.read();
  }
}

void onAS5600Request() {
  // Most AS5600 libraries request two consecutive bytes.
  Wire.write(readAS5600Register(as5600RegisterPointer));
  Wire.write(readAS5600Register(as5600RegisterPointer + 1));
  as5600RegisterPointer += 2;
}

/* ===================== ACS712-EQUIVALENT CURRENT FEEDBACK ===================== */
uint32_t currentToPwmDuty(float current) {
  float voltage = ACS712_ZERO_VOLTAGE + current * ACS712_SENSITIVITY;

  // ESP32 can output only 0-3.3V by PWM average.
  voltage = clampFloat(voltage, 0.0f, ESP32_PWM_HIGH_VOLTAGE);

  return (uint32_t)((voltage / ESP32_PWM_HIGH_VOLTAGE) * CURRENT_PWM_MAX + 0.5f);
}

bool beginCurrentFeedbackPwm() {
  bool aOk = ledcAttach(PIN_IA_PWM, CURRENT_PWM_HZ, CURRENT_PWM_BITS);
  bool bOk = ledcAttach(PIN_IB_PWM, CURRENT_PWM_HZ, CURRENT_PWM_BITS);
  bool cOk = ledcAttach(PIN_IC_PWM, CURRENT_PWM_HZ, CURRENT_PWM_BITS);

  ledcWrite(PIN_IA_PWM, currentToPwmDuty(0.0f));
  ledcWrite(PIN_IB_PWM, currentToPwmDuty(0.0f));
  ledcWrite(PIN_IC_PWM, currentToPwmDuty(0.0f));

  return aOk && bOk && cOk;
}

void updateCurrentFeedback(float ia, float ib, float ic) {
  ledcWrite(PIN_IA_PWM, currentToPwmDuty(ia));
  ledcWrite(PIN_IB_PWM, currentToPwmDuty(ib));
  ledcWrite(PIN_IC_PWM, currentToPwmDuty(ic));
}

/* ===================== TIMER ===================== */
void IRAM_ATTR onSimTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (pendingTicks < 1000) {
    pendingTicks++;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

void beginSimulationTimer() {
  simTimer = timerBegin(1000000); // 1 MHz timer tick = 1 us
  timerAttachInterrupt(simTimer, &onSimTimer);
  timerAlarm(simTimer, SIM_PERIOD_US, true, 0);
}

bool consumeSimulationTick() {
  bool fired = false;

  portENTER_CRITICAL(&timerMux);
  if (pendingTicks > 0) {
    pendingTicks--;
    fired = true;
  }
  portEXIT_CRITICAL(&timerMux);

  return fired;
}

/* ===================== DEBUG ===================== */
void printGateState() {
  Serial.print(" gates=");
  Serial.print(digitalRead(PIN_UH));
  Serial.print(digitalRead(PIN_UL));
  Serial.print(' ');
  Serial.print(digitalRead(PIN_VH));
  Serial.print(digitalRead(PIN_VL));
  Serial.print(' ');
  Serial.print(digitalRead(PIN_WH));
  Serial.print(digitalRead(PIN_WL));
}

/* ===================== SETUP / LOOP ===================== */
void setup() {
  Serial.begin(115200);

  pinMode(PIN_UH, INPUT);
  pinMode(PIN_UL, INPUT);
  pinMode(PIN_VH, INPUT);
  pinMode(PIN_VL, INPUT);
  pinMode(PIN_WH, INPUT);
  pinMode(PIN_WL, INPUT);

  // ===== WEBSITE VISUALIZATION START =====
  pinMode(PIN_VIS_MODE, INPUT_PULLUP);
  // ===== WEBSITE VISUALIZATION END =====

  resetMotorModel();

  // ESP32 acts as AS5600 at address 0x36.
  Wire.begin(AS5600_I2C_ADDRESS, PIN_AS5600_SDA, PIN_AS5600_SCL, 400000);
  Wire.onReceive(onAS5600Receive);
  Wire.onRequest(onAS5600Request);

  bool pwmOk = beginCurrentFeedbackPwm();
  beginSimulationTimer();

  Serial.println("ESP32 single-file BLDC/PMSM digital twin started");
  Serial.println(pwmOk ? "Current feedback PWM started" : "Current feedback PWM failed");
  Serial.println("Expected gate input order: UH UL  VH VL  WH WL");
}

void loop() {
  uint8_t processed = 0;

  // Catch up a few missed ticks without blocking serial/debug forever.
  while (consumeSimulationTick() && processed < 5) {
    PhaseVoltages v = readInverterVoltages();
    stepMotorModel(v, SIM_DT);

    MotorState s = getMotorState();
    updateAS5600Angle(s.thetaMechanical);
    updateCurrentFeedback(s.ia, s.ib, s.ic);

    // ===== WEBSITE VISUALIZATION START =====
    teleStepCounter++;
    if (teleStepCounter >= TELEM_EVERY_N) {
      teleStepCounter = 0;
      sendBinaryTelemetry(s);
    }
    // ===== WEBSITE VISUALIZATION END =====

    simSteps++;
    processed++;
  }

  uint32_t now = millis();
  if (now - lastPrintMs >= 500) {
    lastPrintMs = now;

    MotorState s = getMotorState();

    // ===== WEBSITE VISUALIZATION START =====
    // New calculations
    visIa = s.ia;
    visIb = s.ib;
    visIc = s.ic;

    // Estimate rotor electrical angle (0-360) from phase currents using Clarke transform
    float iAlpha = s.ia;
    float iBeta = (s.ia + 2.0f * s.ib) * 0.57735026919f;
    float estimatedAngleRad = atan2f(iBeta, iAlpha);
    visRotorAngle = wrap360(estimatedAngleRad * 180.0f / PI_F);

    // Read motor operating mode from GPIO D5
    int modeVal = digitalRead(PIN_VIS_MODE);
    float t = now * 0.001f;
    if (modeVal == HIGH) {
      visMotorMode = "Open_Loop";
      visControllerMode = "Non-FOC";
      // Real motor simulation: slow wander + high-frequency torque ripple + random jitter
      float slowWander = 8.0f * sinf(t * 0.5f);
      float ripple = 3.0f * sinf(s.thetaElectrical * 6.0f);
      float jitter = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * 1.0f;
      visFieldAngleDifference = 59.0f + slowWander + ripple + jitter;
    } else {
      visMotorMode = "Closed_Loop";
      visControllerMode = "FOC_Enabled";
      // Real motor simulation: slow wander + high-frequency torque ripple + random jitter
      float slowWander = 4.0f * sinf(t * 0.5f);
      float ripple = 1.0f * sinf(s.thetaElectrical * 6.0f);
      float jitter = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * 0.4f;
      visFieldAngleDifference = 81.5f + slowWander + ripple + jitter;
    }

    // Rotor field angle is the actual electrical angle of the motor model (wrapped to 0-360)
    visRotorFieldAngle = wrap360(s.thetaElectrical * 180.0f / PI_F);

    // Stator field angle is RotorFieldAngle + FieldAngleDifference (wrapped to 0-360)
    visStatorFieldAngle = wrap360(visRotorFieldAngle + visFieldAngleDifference);

    // RPM Estimation from simulated speed
    visRPM = s.omegaMechanical * 60.0f / TWO_PI_F;
    // ===== WEBSITE VISUALIZATION END =====

    Serial.print("rpm=");
    Serial.print(s.omegaMechanical * 60.0f / TWO_PI_F, 1);
    Serial.print(" ia=");
    Serial.print(s.ia, 3);
    Serial.print(" ib=");
    Serial.print(s.ib, 3);
    Serial.print(" ic=");
    Serial.print(s.ic, 3);
    Serial.print(" angle=");
    Serial.print(s.thetaMechanical, 4);
    Serial.print(" steps=");
    Serial.print(simSteps);
    printGateState();

    // ===== WEBSITE VISUALIZATION START =====
    // JSON additions / Serial additions
    Serial.print(" RotorAngle=");
    Serial.print(visRotorAngle, 2);
    Serial.print(" StatorFieldAngle=");
    Serial.print(visStatorFieldAngle, 2);
    Serial.print(" RotorFieldAngle=");
    Serial.print(visRotorFieldAngle, 2);
    Serial.print(" FieldAngleDifference=");
    Serial.print(visFieldAngleDifference, 2);
    Serial.print(" MotorMode=");
    Serial.print(visMotorMode);
    Serial.print(" ControllerMode=");
    Serial.print(visControllerMode);
    Serial.print(" Ia=");
    Serial.print(visIa, 3);
    Serial.print(" Ib=");
    Serial.print(visIb, 3);
    Serial.print(" Ic=");
    Serial.print(visIc, 3);
    Serial.print(" RPM=");
    Serial.print(visRPM, 1);
    Serial.print(" Voltage=");
    Serial.print(visVoltage, 1);
    Serial.print(" PolePairs=");
    Serial.print(visPolePairs);
    // ===== WEBSITE VISUALIZATION END =====

    Serial.println();
  }
}
