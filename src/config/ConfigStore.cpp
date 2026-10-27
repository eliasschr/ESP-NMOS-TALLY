#include "config/ConfigStore.h"

#include <LittleFS.h>

#include "config/ConfigRecord.h"
#include "config/ConfigRecovery.h"

namespace config {
namespace {
bool hasValidRecord(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  ConfigRecord record = {};
  const bool complete = file.size() == sizeof(record) &&
                        file.readBytes(reinterpret_cast<char*>(&record), sizeof(record)) == sizeof(record);
  file.close();
  return complete && isRecordValid(record);
}
}  // namespace

bool Store::begin() { return LittleFS.begin(); }

bool Store::load(DeviceConfig& value) {
  value = defaults(defaultLedPin_);
  ConfigRecord records[2] = {};
  bool valid[2] = {};
  const char* paths[] = {kPath, kBackupPath};
  for (size_t index = 0; index < 2; ++index) {
    const char* path = paths[index];
    File file = LittleFS.open(path, "r");
    if (!file) continue;
    const bool complete = file.size() == sizeof(records[index]) &&
                          file.readBytes(reinterpret_cast<char*>(&records[index]), sizeof(records[index])) == sizeof(records[index]);
    file.close();
    valid[index] = complete && isRecordValid(records[index]);
  }
  const RecoverySource source = selectRecoverySource(valid[0], valid[1]);
  if (source == RecoverySource::Primary || source == RecoverySource::Backup) {
    value = records[source == RecoverySource::Primary ? 0 : 1].configuration;
    return true;
  }
  return false;
}

bool Store::save(const DeviceConfig& value) {
  if (!isValid(value)) return false;
  const ConfigRecord record = makeRecord(value);
  File file = LittleFS.open("/config.tmp", "w");
  if (!file) return false;
  const bool complete = file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record)) == sizeof(record);
  file.close();
  if (!complete) {
    LittleFS.remove("/config.tmp");
    return false;
  }
  // Keep the last valid record until the replacement has been installed. A
  // sudden power loss can therefore leave either the old record or a recoverable
  // backup instead of an empty configuration.
  const bool hadPrevious = LittleFS.exists(kPath);
  if (hadPrevious && hasValidRecord(kPath)) {
    LittleFS.remove(kBackupPath);
    if (!LittleFS.rename(kPath, kBackupPath)) return false;
  } else if (hadPrevious && !LittleFS.remove(kPath)) {
    return false;
  }
  if (!LittleFS.rename("/config.tmp", kPath)) {
    if (hadPrevious && LittleFS.exists(kBackupPath)) LittleFS.rename(kBackupPath, kPath);
    return false;
  }
  return true;
}

}  // namespace config
