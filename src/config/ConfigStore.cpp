#include "config/ConfigStore.h"

#include <LittleFS.h>

namespace config {

bool Store::begin() { return LittleFS.begin(); }

bool Store::load(DeviceConfig& value) {
  value = defaults(defaultLedPin_);
  File file = LittleFS.open(kPath, "r");
  if (!file) return false;
  const bool complete = file.size() == sizeof(DeviceConfig) &&
                        file.readBytes(reinterpret_cast<char*>(&value), sizeof(value)) == sizeof(value);
  file.close();
  if (!complete || !isValid(value)) {
    value = defaults(defaultLedPin_);
    return false;
  }
  return true;
}

bool Store::save(const DeviceConfig& value) {
  if (!isValid(value)) return false;
  File file = LittleFS.open("/config.tmp", "w");
  if (!file) return false;
  const bool complete = file.write(reinterpret_cast<const uint8_t*>(&value), sizeof(value)) == sizeof(value);
  file.close();
  if (!complete) {
    LittleFS.remove("/config.tmp");
    return false;
  }
  // Keep the last valid record until the replacement has been installed. A
  // sudden power loss can therefore leave either the old record or a recoverable
  // backup instead of an empty configuration.
  LittleFS.remove("/config.bak");
  const bool hadPrevious = LittleFS.exists(kPath);
  if (hadPrevious && !LittleFS.rename(kPath, "/config.bak")) return false;
  if (!LittleFS.rename("/config.tmp", kPath)) {
    if (hadPrevious) LittleFS.rename("/config.bak", kPath);
    return false;
  }
  LittleFS.remove("/config.bak");
  return true;
}

}  // namespace config
