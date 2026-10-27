#include "config/DeviceConfig.h"

#include <string.h>

namespace config {
namespace {
bool bounded(const char* text, size_t capacity) {
  return text != nullptr && strnlen(text, capacity) < capacity;
}
}

DeviceConfig defaults(uint8_t ledPin) {
  DeviceConfig value = {};
  value.schemaVersion = kSchemaVersion;
  copyBounded(value.deviceName, sizeof(value.deviceName), "ESP NMOS Tally");
  value.brightness = kDefaultBrightness;
  value.ledPin = ledPin;
  return value;
}

bool isValid(const DeviceConfig& value) {
  return value.schemaVersion == kSchemaVersion &&
         bounded(value.deviceName, sizeof(value.deviceName)) &&
         bounded(value.wifiSsid, sizeof(value.wifiSsid)) &&
         bounded(value.wifiPassword, sizeof(value.wifiPassword)) &&
         value.deviceName[0] != '\0' && value.brightness <= kMaximumBrightness;
}

bool hasNetwork(const DeviceConfig& value) { return isValid(value) && value.wifiSsid[0] != '\0'; }

bool copyBounded(char* destination, size_t capacity, const char* source) {
  if (destination == nullptr || source == nullptr || capacity == 0) return false;
  const size_t length = strnlen(source, capacity);
  if (length >= capacity) return false;
  memcpy(destination, source, length + 1);
  return true;
}

uint8_t clampBrightness(uint8_t requested) {
  return requested > kMaximumBrightness ? kMaximumBrightness : requested;
}

bool networkSettingsChanged(const DeviceConfig& current, const DeviceConfig& next) {
  return strcmp(current.wifiSsid, next.wifiSsid) != 0 ||
         strcmp(current.wifiPassword, next.wifiPassword) != 0;
}

bool equals(const DeviceConfig& left, const DeviceConfig& right) {
  return memcmp(&left, &right, sizeof(DeviceConfig)) == 0;
}

}  // namespace config
