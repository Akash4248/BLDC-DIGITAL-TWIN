#pragma once
#include <Arduino.h>
#include "MotorState.h"

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

class TelemetryManager {
 public:
  TelemetryManager();
  void send(const MotorState& state);

 private:
  uint16_t seqNum_;
  uint16_t calculateCRC(const uint8_t* data, size_t len);
};
