#include "TwinLink.h"

namespace TwinProtocol {

TwinLink::TwinLink(HardwareSerial& serial, uint32_t timeoutMs)
    : serial_(serial),
      timeoutMs_(timeoutMs),
      lastRxTime_(0),
      connected_(false),
      rxCount_(0),
      txCount_(0),
      errorCount_(0) {
  memset(&latestTelemetry_, 0, sizeof(latestTelemetry_));
}

void TwinLink::begin(unsigned long baudRate) {
  serial_.begin(baudRate);
  parser_.reset();
}

bool TwinLink::update() {
  bool newPacketReceived = false;
  
  // Read incoming bytes from the hardware ring buffer (via DMA/Interrupts internally)
  while (serial_.available() > 0) {
    uint8_t byte = serial_.read();
    FrameParser::Result result;
    
    if (parser_.feed(byte, result)) {
      if (result.valid) {
        if (result.type == kTypeTelemetry) {
          latestTelemetry_ = result.telemetry;
          lastRxTime_ = millis();
          connected_ = true;
          rxCount_++;
          newPacketReceived = true;
        }
      } else {
        errorCount_++;
      }
    }
  }
  
  // Check timeout to implement disconnect logic
  if (connected_ && (millis() - lastRxTime_ > timeoutMs_)) {
    connected_ = false;
    // Optionally zero out control signals on disconnect for safety
    latestTelemetry_.dutyA = 0.0f;
    latestTelemetry_.dutyB = 0.0f;
    latestTelemetry_.dutyC = 0.0f;
  }
  
  return newPacketReceived;
}

bool TwinLink::isConnected() const {
  return connected_;
}

const TelemetryPacket& TwinLink::getLatestTelemetry() const {
  return latestTelemetry_;
}

bool TwinLink::sendTwinState(const TwinStatePacket& packet) {
  size_t len = encodeTwinState(packet, txBuffer_, sizeof(txBuffer_));
  if (len > 0) {
    size_t written = serial_.write(txBuffer_, len);
    if (written == len) {
      txCount_++;
      return true;
    } else {
      errorCount_++; // Write overflow
    }
  }
  return false;
}

} // namespace TwinProtocol
