#include "TelemetryManager.h"
#include "Config.h"

TelemetryManager::TelemetryManager() : seqNum_(0) {}

uint16_t TelemetryManager::calculateCRC(const uint8_t* data, size_t len) {
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

void TelemetryManager::send(const MotorState& state) {
  TwinStatePacket pkt;
  pkt.header.sync[0] = 0xAA;
  pkt.header.sync[1] = 0x55;
  pkt.header.version = 1;
  pkt.header.packet_type = 2; // TwinState
  pkt.header.seq = seqNum_++;
  pkt.header.payload_len = sizeof(TwinStatePayload); // 47
  pkt.header.reserved = 0;

  pkt.payload.timestamp_us = esp_timer_get_time();
  pkt.payload.ia = state.ia;
  pkt.payload.ib = state.ib;
  pkt.payload.ic = state.ic;
  pkt.payload.rotor_speed = state.mechSpeedRad;
  pkt.payload.rotor_angle = state.mechAngle;
  pkt.payload.electrical_angle = state.elecAngle;
  pkt.payload.torque = state.torqueElec;
  pkt.payload.temperature = state.temperature;
  
  // Pack Hall state bits
  pkt.payload.hall = (state.hallA & 0x01) | 
                     ((state.hallB & 0x01) << 1) | 
                     ((state.hallC & 0x01) << 2);
                     
  pkt.payload.encoder = state.encoderRawAngle;
  pkt.payload.fault_flags = state.faultFlags;

  // CRC is computed over header[2:] + payload
  // offset 2 skips sync (2 bytes), length to check is 8 (header remainder) + 47 (payload) = 55 bytes
  uint8_t* crc_start = ((uint8_t*)&pkt) + 2;
  pkt.crc = calculateCRC(crc_start, 8 + sizeof(TwinStatePayload));

  Serial.write((uint8_t*)&pkt, sizeof(TwinStatePacket));
}
