#include "Protocol.h"

#include <string.h>

namespace TwinProtocol {

uint16_t crc16CcittFalse(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x8000) {
        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

static void writeU16LE(uint8_t* out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value & 0xFF);
  out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

static void writeU64LE(uint8_t* out, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out[i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
  }
}

static void writeF32LE(uint8_t* out, float value) {
  memcpy(out, &value, sizeof(float));
}

size_t encodeTelemetry(const TelemetryPacket& packet, uint8_t* out, size_t outSize) {
  constexpr uint16_t payloadLen = 24;
  constexpr size_t frameLen = kHeaderSize + payloadLen + kCrcSize;
  if (outSize < frameLen) {
    return 0;
  }

  out[0] = kSync0;
  out[1] = kSync1;
  out[2] = kVersion;
  out[3] = kTypeTelemetry;
  writeU16LE(out + 4, packet.sequence);
  writeU16LE(out + 6, payloadLen);
  writeU16LE(out + 8, 0);

  uint8_t* payload = out + kHeaderSize;
  writeU64LE(payload + 0, packet.timestampUs);
  writeF32LE(payload + 8, packet.dutyA);
  writeF32LE(payload + 12, packet.dutyB);
  writeF32LE(payload + 16, packet.dutyC);
  writeF32LE(payload + 20, packet.vdc);

  const uint16_t crc = crc16CcittFalse(out + 2, kHeaderSize - 2 + payloadLen);
  writeU16LE(out + kHeaderSize + payloadLen, crc);
  return frameLen;
}

size_t encodeTwinState(const TwinStatePacket& packet, uint8_t* out, size_t outSize) {
  constexpr uint16_t payloadLen = 20;
  constexpr size_t frameLen = kHeaderSize + payloadLen + kCrcSize;
  if (outSize < frameLen) {
    return 0;
  }

  out[0] = kSync0;
  out[1] = kSync1;
  out[2] = kVersion;
  out[3] = kTypeTwinState;
  writeU16LE(out + 4, packet.sequence);
  writeU16LE(out + 6, payloadLen);
  writeU16LE(out + 8, 0);

  uint8_t* payload = out + kHeaderSize;
  writeF32LE(payload + 0, packet.ia);
  writeF32LE(payload + 4, packet.ib);
  writeF32LE(payload + 8, packet.ic);
  writeF32LE(payload + 12, packet.rotorAngle);
  writeF32LE(payload + 16, packet.rotorSpeed);

  const uint16_t crc = crc16CcittFalse(out + 2, kHeaderSize - 2 + payloadLen);
  writeU16LE(out + kHeaderSize + payloadLen, crc);
  return frameLen;
}

FrameParser::FrameParser() { reset(); }

void FrameParser::reset() {
  length_ = 0;
  memset(buffer_, 0, sizeof(buffer_));
}

void FrameParser::consumeByte(uint8_t byte) {
  if (length_ < sizeof(buffer_)) {
    buffer_[length_++] = byte;
  } else {
    memmove(buffer_, buffer_ + 1, sizeof(buffer_) - 1);
    buffer_[sizeof(buffer_) - 1] = byte;
    length_ = sizeof(buffer_);
  }
}

void FrameParser::resync() {
  if (length_ == 0) {
    return;
  }
  memmove(buffer_, buffer_ + 1, length_ - 1);
  --length_;
}

bool FrameParser::tryParseFrame(Result& result) {
  while (length_ >= 2 && !(buffer_[0] == kSync0 && buffer_[1] == kSync1)) {
    resync();
  }

  if (length_ < kHeaderSize) {
    return false;
  }

  const uint8_t version = buffer_[2];
  const uint8_t type = buffer_[3];
  const uint16_t payloadLen = static_cast<uint16_t>(buffer_[6]) |
                              (static_cast<uint16_t>(buffer_[7]) << 8);
  const size_t frameLen = kHeaderSize + payloadLen + kCrcSize;

  if (version != kVersion) {
    resync();
    return false;
  }

  if (type != kTypeTelemetry && type != kTypeTwinState) {
    resync();
    return false;
  }

  if ((type == kTypeTelemetry && payloadLen != 24) ||
      (type == kTypeTwinState && payloadLen != 20)) {
    resync();
    return false;
  }

  if (length_ < frameLen) {
    return false;
  }

  const uint16_t receivedCrc = static_cast<uint16_t>(buffer_[frameLen - 2]) |
                               (static_cast<uint16_t>(buffer_[frameLen - 1]) << 8);
  const uint16_t calculatedCrc = crc16CcittFalse(buffer_ + 2, kHeaderSize - 2 + payloadLen);
  if (receivedCrc != calculatedCrc) {
    resync();
    return false;
  }

  const uint8_t* payload = buffer_ + kHeaderSize;
  result.valid = true;
  result.type = type;
  result.telemetry.sequence = static_cast<uint16_t>(buffer_[4]) | (static_cast<uint16_t>(buffer_[5]) << 8);
  result.twinState.sequence = result.telemetry.sequence;

  if (type == kTypeTelemetry) {
    memcpy(&result.telemetry.timestampUs, payload + 0, sizeof(uint64_t));
    memcpy(&result.telemetry.dutyA, payload + 8, sizeof(float));
    memcpy(&result.telemetry.dutyB, payload + 12, sizeof(float));
    memcpy(&result.telemetry.dutyC, payload + 16, sizeof(float));
    memcpy(&result.telemetry.vdc, payload + 20, sizeof(float));
  } else {
    memcpy(&result.twinState.ia, payload + 0, sizeof(float));
    memcpy(&result.twinState.ib, payload + 4, sizeof(float));
    memcpy(&result.twinState.ic, payload + 8, sizeof(float));
    memcpy(&result.twinState.rotorAngle, payload + 12, sizeof(float));
    memcpy(&result.twinState.rotorSpeed, payload + 16, sizeof(float));
  }

  memmove(buffer_, buffer_ + frameLen, length_ - frameLen);
  length_ -= frameLen;
  return true;
}

bool FrameParser::feed(uint8_t byte, Result& result) {
  consumeByte(byte);
  return tryParseFrame(result);
}

}  // namespace TwinProtocol

