#pragma once

#include <stddef.h>
#include <stdint.h>

namespace config {

constexpr size_t kDeviceNameLength = 32;
constexpr size_t kSsidLength = 32;
constexpr size_t kPasswordLength = 64;
constexpr uint16_t kSchemaVersion = 2;
constexpr uint8_t kDefaultBrightness = 16;
// This is intentionally conservative for a 32-pixel FeatherWing. It limits
// rendered intensity but is not an electrical current guarantee.
constexpr uint8_t kMaximumBrightness = 32;

struct DeviceConfig {
  uint16_t schemaVersion;
  char deviceName[kDeviceNameLength + 1];
  char wifiSsid[kSsidLength + 1];
  char wifiPassword[kPasswordLength + 1];
  uint8_t brightness;
  uint8_t ledPin;
};

DeviceConfig defaults(uint8_t ledPin);
bool isValid(const DeviceConfig& value);
bool hasNetwork(const DeviceConfig& value);
bool copyBounded(char* destination, size_t capacity, const char* source);
uint8_t clampBrightness(uint8_t requested);
bool networkSettingsChanged(const DeviceConfig& current, const DeviceConfig& next);
bool equals(const DeviceConfig& left, const DeviceConfig& right);

}  // namespace config
