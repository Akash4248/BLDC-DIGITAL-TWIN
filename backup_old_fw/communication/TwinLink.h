#pragma once

#include <Arduino.h>
#include "Protocol.h"

namespace TwinProtocol {

class TwinLink {
 public:
  // Initialize with a HardwareSerial interface and an optional timeout (ms)
  TwinLink(HardwareSerial& serial, uint32_t timeoutMs = 100);

  // Initialize the serial port with the given baud rate
  void begin(unsigned long baudRate);

  // Call this in the main loop to process incoming bytes
  // Returns true if a valid packet was just received
  bool update();

  // Returns true if the link is active (packets received recently)
  bool isConnected() const;

  // Get the latest telemetry packet received from the controller (ESP32)
  const TelemetryPacket& getLatestTelemetry() const;

  // Send a TwinStatePacket back to the controller
  bool sendTwinState(const TwinStatePacket& packet);

  // Statistics
  uint32_t getRxCount() const { return rxCount_; }
  uint32_t getTxCount() const { return txCount_; }
  uint32_t getErrorCount() const { return errorCount_; }

 private:
  HardwareSerial& serial_;
  uint32_t timeoutMs_;
  
  FrameParser parser_;
  TelemetryPacket latestTelemetry_;
  
  uint32_t lastRxTime_;
  bool connected_;
  
  uint32_t rxCount_;
  uint32_t txCount_;
  uint32_t errorCount_;
  
  // Static buffer for encoding outgoing packets to avoid dynamic allocation
  uint8_t txBuffer_[64];
};

} // namespace TwinProtocol
