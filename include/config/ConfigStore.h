#pragma once

#include "config/DeviceConfig.h"

namespace config {

class Store {
 public:
  static constexpr const char* kPath = "/config.bin";

  explicit Store(uint8_t defaultLedPin) : defaultLedPin_(defaultLedPin) {}
  bool begin();
  bool load(DeviceConfig& value);
  bool save(const DeviceConfig& value);

 private:
  uint8_t defaultLedPin_;
};

}  // namespace config
