#pragma once

#include <stdint.h>

#include "config/DeviceConfig.h"

namespace config {

constexpr uint32_t kRecordMagic = 0x54414C59UL;  // "TALY"
constexpr uint16_t kRecordVersion = 1;

// The checksum detects accidental corruption and interrupted/corrupt writes.
// It is not authentication and does not protect stored Wi-Fi credentials from
// an attacker with physical flash access.
struct ConfigRecord {
  uint32_t magic;
  uint16_t recordVersion;
  DeviceConfig configuration;
  uint32_t checksum;
};

ConfigRecord makeRecord(const DeviceConfig& configuration);
bool isRecordValid(const ConfigRecord& record);
uint32_t recordChecksum(const ConfigRecord& record);

}  // namespace config
