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
  value.brightness = 64;
  value.ledPin = ledPin;
  return value;
}

bool isValid(const DeviceConfig& value) {
  return value.schemaVersion == kSchemaVersion &&
         bounded(value.deviceName, sizeof(value.deviceName)) &&
         bounded(value.wifiSsid, sizeof(value.wifiSsid)) &&
         bounded(value.wifiPassword, sizeof(value.wifiPassword)) &&
         value.deviceName[0] != '\0';
}

bool hasNetwork(const DeviceConfig& value) { return isValid(value) && value.wifiSsid[0] != '\0'; }

bool copyBounded(char* destination, size_t capacity, const char* source) {
  if (destination == nullptr || source == nullptr || capacity == 0) return false;
  const size_t length = strnlen(source, capacity);
  if (length >= capacity) return false;
  memcpy(destination, source, length + 1);
  return true;
}

}  // namespace config
