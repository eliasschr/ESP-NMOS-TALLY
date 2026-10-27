#include "config/ConfigRecord.h"

namespace config {
namespace {
uint32_t crc32(const uint8_t* bytes, size_t count) {
  uint32_t value = 0xFFFFFFFFUL;
  for (size_t index = 0; index < count; ++index) {
    value ^= bytes[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      value = (value >> 1) ^ ((value & 1U) ? 0xEDB88320UL : 0U);
    }
  }
  return ~value;
}
}  // namespace

ConfigRecord makeRecord(const DeviceConfig& configuration) {
  ConfigRecord record = {};
  record.magic = kRecordMagic;
  record.recordVersion = kRecordVersion;
  record.configuration = configuration;
  record.checksum = recordChecksum(record);
  return record;
}

uint32_t recordChecksum(const ConfigRecord& record) {
  return crc32(reinterpret_cast<const uint8_t*>(&record), offsetof(ConfigRecord, checksum));
}

bool isRecordValid(const ConfigRecord& record) {
  return record.magic == kRecordMagic && record.recordVersion == kRecordVersion &&
         record.checksum == recordChecksum(record) && isValid(record.configuration);
}

}  // namespace config
