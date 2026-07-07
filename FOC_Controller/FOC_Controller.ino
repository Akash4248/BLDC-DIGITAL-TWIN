#include <Arduino.h>

#define HIL_MODE 1 // Set to 1 for ESP32 Digital Twin, 0 for Real Motor with AS5600

#if !HIL_MODE
  #include <Wire.h>
  #include <AS5600.h>
  AS5600 encoder;
#endif

/* ===================== PIN DEFINITIONS ===================== */
#define UH 3
#define UL 5
#define VH 6
#define VL 9
#define WH 10
#define WL 11

#define IA_PIN A0
#define IB_PIN A1
#define IC_PIN A2

#if HIL_MODE
  // Quadrature Encoder Pins from ESP32
  #define ENC_A 2 
  #define ENC_B 4 
  volatile long encoderTicks = 0;
  const float PPR = 36.0f; // ESP32 Twin PPR
#endif

/* ===================== MOTOR / SYSTEM PARAMETERS ===================== */
const float polePairs = 2.0f;
const float supplyVoltage = 12.0f;

/* Serial-entered speed limits */
const float minTargetRPM = 0.0f;
const float maxTargetRPM = 800.0f;

/* Voltage/current limits */
const float maxVd = 2.0f;
const float maxVq = 6.0f;
const float maxVoltageVector = 6.0f;
const float maxIqRef = 4.0f;
const float id_ref_fixed = 0.0f;

/* ===================== CURRENT SENSOR PARAMETERS ===================== */
const float adcRefVoltage = 5.0f;
const float adcMaxCount   = 1023.0f;

#if HIL_MODE
  // ESP32 PWM outputs 0-3.3V which maps to -10A to +10A (20A span)
  // Sensitivity = 3.3V / 20A = 0.165 V/A
  const float currentSensitivity = 0.165f; 
#else
  // ACS712 20A = 100 mV/A
  const float currentSensitivity = 0.100f;   
#endif

/* Calibrated offsets */
float currentMidVoltageA = 2.5f;
float currentMidVoltageB = 2.5f;
float currentMidVoltageC = 2.5f;

/* ===================== CONTROLLER GAINS ===================== */
float Kp_speed = 5.2666f;
float Ki_speed = 1.3209f;

float Kp_id = 0.2891f;
float Ki_id = 0.0781f;

float Kp_iq = 0.2891f;
float Ki_iq = 0.0781f;

/* ===================== GLOBAL VARIABLES ===================== */
float electrical_offset = 0.0f;

float mechAngle = 0.0f;
float elecAngle = 0.0f;

float mechSpeedRad = 0.0f;
float mechSpeedRPM = 0.0f;
float filteredMechSpeedRad = 0.0f;

float prevMechAngle = 0.0f;

float targetRPM = 0.0f;
float targetSpeedRad = 0.0f;

float ia = 0.0f, ib = 0.0f, ic = 0.0f;
float ialpha = 0.0f, ibeta = 0.0f;
float id = 0.0f, iq = 0.0f;

float iq_ref = 0.0f;
float vd = 0.0f;
float vq = 0.0f;

float integ_speed = 0.0f;
float integ_id = 0.0f;
float integ_iq = 0.0f;

unsigned long lastControlMicros = 0;
unsigned long lastSpeedMicros = 0;
unsigned long lastPrintMillis = 0;

/* Diagnostics */
float speedErrorRPM = 0.0f;
float speedErrorPercent = 0.0f;

/* ===================== UTILITY ===================== */
float wrapAngle(float x)
{
  while (x >= 2.0f * PI) x -= 2.0f * PI;
  while (x < 0.0f)       x += 2.0f * PI;
  return x;
}

/* ===================== ENCODER ===================== */
#if HIL_MODE
  void isr_encA() {
    bool a = digitalRead(ENC_A);
    bool b = digitalRead(ENC_B);
    if (a == b) encoderTicks++;
    else encoderTicks--;
  }

  ISR(PCINT2_vect) {
    bool a = digitalRead(ENC_A);
    bool b = digitalRead(ENC_B);
    if (a != b) encoderTicks++;
    else encoderTicks--;
  }
#endif

float mechanicalAngle()
{
#if HIL_MODE
  long ticks;
  noInterrupts();
  ticks = encoderTicks;
  interrupts();
  // 1 mech revolution = PPR * 4 ticks (quadrature)
  float angle = fmod((float)ticks * (2.0f * PI / (PPR * 4.0f)), 2.0f * PI);
  if (angle < 0.0f) angle += 2.0f * PI;
  return angle;
#else
  int raw = encoder.readAngle();
  return (raw * 2.0f * PI) / 4096.0f;
#endif
}

float electricalAngle()
{
  float mech = mechanicalAngle();
  float elec = mech * polePairs - electrical_offset;
  return wrapAngle(elec);
}

/* ===================== ACS712 / HIL FUNCTIONS ===================== */
float adcToVoltage(int adcValue)
{
  return (adcValue / adcMaxCount) * adcRefVoltage;
}

float adcToCurrent(int adcValue, float midVoltage)
{
  float voltage = adcToVoltage(adcValue);
  return (voltage - midVoltage) / currentSensitivity;
}

void calibrateCurrentSensors()
{
  Serial.println("Calibrating Current Offsets...");

  const int samples = 1000;
  long sumA = 0;
  long sumB = 0;
  long sumC = 0;

  for (int i = 0; i < samples; i++)
  {
    sumA += analogRead(IA_PIN);
    sumB += analogRead(IB_PIN);
    sumC += analogRead(IC_PIN);
    delay(2);
  }

  float avgA = (float)sumA / samples;
  float avgB = (float)sumB / samples;
  float avgC = (float)sumC / samples;

  currentMidVoltageA = adcToVoltage((int)avgA);
  currentMidVoltageB = adcToVoltage((int)avgB);
  currentMidVoltageC = adcToVoltage((int)avgC);

  Serial.print("Offset A (V): "); Serial.println(currentMidVoltageA, 4);
  Serial.print("Offset B (V): "); Serial.println(currentMidVoltageB, 4);
  Serial.print("Offset C (V): "); Serial.println(currentMidVoltageC, 4);
}

/* ===================== SERIAL SPEED INPUT ===================== */
void updateSpeedReferenceFromSerial()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0)
    {
      float enteredRPM = input.toFloat();

      if (enteredRPM < minTargetRPM) enteredRPM = minTargetRPM;
      if (enteredRPM > maxTargetRPM) enteredRPM = maxTargetRPM;

      targetRPM = enteredRPM;
      targetSpeedRad = targetRPM * 2.0f * PI / 60.0f;

      Serial.print("New TargetRPM set to: ");
      Serial.println(targetRPM, 1);
    }
  }
}

/* ===================== CURRENT READING ===================== */
void readPhaseCurrents()
{
  int rawA = analogRead(IA_PIN);
  int rawB = analogRead(IB_PIN);
  int rawC = analogRead(IC_PIN);

  ia = adcToCurrent(rawA, currentMidVoltageA);
  ib = adcToCurrent(rawB, currentMidVoltageB);
  ic = adcToCurrent(rawC, currentMidVoltageC);
}

/* ===================== TRANSFORMS ===================== */
void clarkeTransform(float ia_in, float ib_in, float ic_in, float *alpha, float *beta)
{
  (void)ic_in;
  *alpha = ia_in;
  *beta  = (ia_in + 2.0f * ib_in) * 0.57735026919f;
}

void parkTransform(float alpha, float beta, float theta, float *d, float *q)
{
  float s = sin(theta);
  float c = cos(theta);

  *d =  alpha * c + beta * s;
  *q = -alpha * s + beta * c;
}

void invPark(float vd_in, float vq_in, float theta, float *alpha, float *beta)
{
  float s = sin(theta);
  float c = cos(theta);

  *alpha = vd_in * c - vq_in * s;
  *beta  = vd_in * s + vq_in * c;
}

/* ===================== PI CONTROLLER ===================== */
float PI_Controller(float error, float Kp, float Ki, float dt, float *integrator, float outLimit)
{
  *integrator += error * dt;

  float out = Kp * error + Ki * (*integrator);

  if (out > outLimit)
  {
    out = outLimit;
    if (error > 0.0f) *integrator -= error * dt;
  }
  else if (out < -outLimit)
  {
    out = -outLimit;
    if (error < 0.0f) *integrator -= error * dt;
  }

  return out;
}

/* ===================== PWM / SVPWM ===================== */
void setPWM(float Ua, float Ub, float Uc)
{
  Ua = constrain(Ua, 0.0f, supplyVoltage);
  Ub = constrain(Ub, 0.0f, supplyVoltage);
  Uc = constrain(Uc, 0.0f, supplyVoltage);

  int dutyA = (int)(Ua / supplyVoltage * 255.0f);
  int dutyB = (int)(Ub / supplyVoltage * 255.0f);
  int dutyC = (int)(Uc / supplyVoltage * 255.0f);

  dutyA = constrain(dutyA, 0, 255);
  dutyB = constrain(dutyB, 0, 255);
  dutyC = constrain(dutyC, 0, 255);

  analogWrite(UH, dutyA);
  analogWrite(VH, dutyB);
  analogWrite(WH, dutyC);

  #if !HIL_MODE
  /* Valid only if external driver handles dead-time */
  analogWrite(UL, 255 - dutyA);
  analogWrite(VL, 255 - dutyB);
  analogWrite(WL, 255 - dutyC);
  #endif
}

void SVPWM(float alpha, float beta)
{
  float Va = alpha;
  float Vb = -0.5f * alpha + 0.8660254f * beta;
  float Vc = -0.5f * alpha - 0.8660254f * beta;

  float vmax = max(Va, max(Vb, Vc));
  float vmin = min(Va, min(Vb, Vc));
  float voffset = 0.5f * (vmax + vmin);

  Va = Va - voffset + supplyVoltage * 0.5f;
  Vb = Vb - voffset + supplyVoltage * 0.5f;
  Vc = Vc - voffset + supplyVoltage * 0.5f;

  setPWM(Va, Vb, Vc);
}

/* ===================== PWM TIMING FIX ===================== */
// By default, Arduino UNO runs Timer0 (Pin 5,6) at 976 Hz Fast PWM, 
// but Timer1 (Pin 9,10) and Timer2 (Pin 3,11) run at 490 Hz Phase-Correct PWM.
// To generate correct FOC Sine Waves, all 3 pins MUST run at the exact same frequency!
// We will configure Timer1 and Timer2 to match Timer0 (976 Hz Fast PWM)
// so that we don't break micros() which relies on Timer0.
void setupPWMFrequencies() {
  // Timer 1 (Pin 9, 10): Fast PWM 8-bit, Prescaler 64
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);

  // Timer 2 (Pin 3, 11): Fast PWM, Prescaler 64
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS22);

  // Synchronize all timers perfectly
  noInterrupts();
  GTCCR = _BV(TSM) | _BV(PSRASY) | _BV(PSRSYNC); // Halt all timers
  
  TCNT0 = 0; // Reset Timer 0 counter
  TCNT1 = 0; // Reset Timer 1 counter
  TCNT2 = 0; // Reset Timer 2 counter
  
  GTCCR = 0; // Release timers to start exactly in sync
  interrupts();
}

/* ===================== SPEED ESTIMATION ===================== */
void updateSpeedEstimate()
{
  unsigned long now = micros();
  float dt = (now - lastSpeedMicros) * 1e-6f;
  if (dt <= 0.0f) return;

  mechAngle = mechanicalAngle();

  float dtheta = mechAngle - prevMechAngle;

  if (dtheta > PI)       dtheta -= 2.0f * PI;
  else if (dtheta < -PI) dtheta += 2.0f * PI;

  mechSpeedRad = dtheta / dt;

  const float alpha = 0.15f;
  filteredMechSpeedRad = alpha * mechSpeedRad + (1.0f - alpha) * filteredMechSpeedRad;

  mechSpeedRPM = filteredMechSpeedRad * 60.0f / (2.0f * PI);

  prevMechAngle = mechAngle;
  lastSpeedMicros = now;
}

/* ===================== ALIGNMENT ===================== */
void alignRotor()
{
  Serial.println("Rotor alignment started...");

  for (int i = 0; i < 3000; i++)
  {
    float alpha, beta;
    invPark(2.0f, 0.0f, 0.0f, &alpha, &beta);
    SVPWM(alpha, beta);
    delay(1);
  }

  delay(500);

  float mech = mechanicalAngle();
  electrical_offset = mech * polePairs;

  Serial.print("Electrical offset = ");
  Serial.println(electrical_offset, 6);

  setPWM(supplyVoltage * 0.5f, supplyVoltage * 0.5f, supplyVoltage * 0.5f);
  delay(200);
}

/* ===================== SETUP ===================== */
void setup()
{
  Serial.begin(115200);
  
  #if !HIL_MODE
    Wire.begin();
  #else
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENC_A), isr_encA, CHANGE);
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT20);
  #endif

  setupPWMFrequencies();

  pinMode(UH, OUTPUT);
  pinMode(VH, OUTPUT);
  pinMode(WH, OUTPUT);
  
  #if !HIL_MODE
  pinMode(UL, OUTPUT);
  pinMode(VL, OUTPUT);
  pinMode(WL, OUTPUT);
  #endif

  pinMode(IA_PIN, INPUT);
  pinMode(IB_PIN, INPUT);
  pinMode(IC_PIN, INPUT);

  delay(1500);

  calibrateCurrentSensors();
  alignRotor();

  prevMechAngle = mechanicalAngle();
  lastSpeedMicros = micros();
  lastControlMicros = micros();
  lastPrintMillis = millis();

  Serial.println("FOC started.");
  Serial.println("Enter target speed in RPM through Serial Monitor.");
  Serial.println("Example: 300 or 800");
}

/* ===================== MAIN LOOP ===================== */
void loop()
{
  updateSpeedReferenceFromSerial();

  unsigned long now = micros();

  if (now - lastControlMicros >= 1000)
  {
    float dt = (now - lastControlMicros) * 1e-6f;
    lastControlMicros = now;

    updateSpeedEstimate();
    elecAngle = electricalAngle();

    readPhaseCurrents();

    clarkeTransform(ia, ib, ic, &ialpha, &ibeta);
    parkTransform(ialpha, ibeta, elecAngle, &id, &iq);

    float speedError = targetSpeedRad - filteredMechSpeedRad;
    iq_ref = PI_Controller(speedError, Kp_speed, Ki_speed, dt, &integ_speed, maxIqRef);

    float err_id = id_ref_fixed - id;
    float err_iq = iq_ref - iq;

    vd = PI_Controller(err_id, Kp_id, Ki_id, dt, &integ_id, maxVd);
    vq = PI_Controller(err_iq, Kp_iq, Ki_iq, dt, &integ_iq, maxVq);

    float mag = sqrt(vd * vd + vq * vq);
    if (mag > maxVoltageVector && mag > 0.0001f)
    {
      float scale = maxVoltageVector / mag;
      vd *= scale;
      vq *= scale;
    }

    float valpha, vbeta;
    invPark(vd, vq, elecAngle, &valpha, &vbeta);

    SVPWM(valpha, vbeta);
  }

  if (millis() - lastPrintMillis >= 200)
  {
    lastPrintMillis = millis();

    speedErrorRPM = targetRPM - mechSpeedRPM;

    if (targetRPM > 1.0f)
      speedErrorPercent = (fabs(speedErrorRPM) / targetRPM) * 100.0f;
    else
      speedErrorPercent = 0.0f;

    Serial.print("TargetRPM: ");
    Serial.print(targetRPM, 1);
    Serial.print("  ActualRPM: ");
    Serial.print(mechSpeedRPM, 1);
    Serial.print("  Error%: ");
    Serial.println(speedErrorPercent, 2);
  }
}
