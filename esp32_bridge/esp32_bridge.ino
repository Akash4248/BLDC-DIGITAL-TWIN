// ==============================================================================
// BLDC DIGITAL TWIN - ESP32 TELEMETRY BRIDGE
// ==============================================================================
// This script simply forwards binary telemetry between the PC (Python Backend)
// and the Arduino Uno (running the 5V Digital Twin core).
// 
// WIRING:
// ESP32 RX2 (Pin 16) <----> Arduino Uno TX (Pin 1) [Use a voltage divider!]
// ESP32 TX2 (Pin 17) <----> Arduino Uno RX (Pin 0)
// ESP32 GND          <----> Arduino Uno GND
// ==============================================================================

#define RXD2 16
#define TXD2 17

void setup() {
  // Connect to PC (Python backend) via USB
  Serial.begin(115200); 

  // Connect to Arduino Uno via Hardware UART
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  // Forward from PC to Arduino
  if (Serial.available()) {
    Serial2.write(Serial.read());
  }

  // Forward from Arduino to PC
  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}
