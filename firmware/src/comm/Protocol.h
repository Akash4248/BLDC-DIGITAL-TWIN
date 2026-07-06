#pragma once

#include <Arduino.h>

namespace TwinProtocol {

constexpr uint8_t kSync0 = 0xAA;
constexpr uint8_t kSync1 = 0x55;
constexpr uint8_t kVersion = 1;
constexpr size_t kHeaderSize = 10;
constexpr size_t kCrcSize = 2;
constexpr uint8_t kTypeTelemetry = 0x01;
constexpr uint8_t kTypeTwinState = 0x02;

struct TelemetryPacket {
  uint16_t sequence;
  uint64_t timestampUs;
  float dutyA;
  float dutyB;
  float dutyC;
  float vdc;
};

struct TwinStatePacket {
  uint16_t sequence;
  float ia;
  float ib;
  float ic;
  float rotorAngle;
  float rotorSpeed;
};

uint16_t crc16CcittFalse(const uint8_t* data, size_t length);

size_t encodeTelemetry(const TelemetryPacket& packet, uint8_t* out, size_t outSize);
size_t encodeTwinState(const TwinStatePacket& packet, uint8_t* out, size_t outSize);

class FrameParser {
 public:
  struct Result {
    bool valid;
    uint8_t type;
    TelemetryPacket telemetry;
    TwinStatePacket twinState;
  };

  FrameParser();
  void reset();
  bool feed(uint8_t byte, Result& result);

 private:
  void consumeByte(uint8_t byte);
  bool tryParseFrame(Result& result);
  void resync();

  uint8_t buffer_[96];
  size_t length_;
};

}  // namespace TwinProtocol

