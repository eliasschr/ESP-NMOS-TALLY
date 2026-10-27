#pragma once

#include <stdint.h>

#include "config/DeviceConfig.h"

namespace network {

enum class Mode : uint8_t { Connecting, Station, Provisioning };

class Controller {
 public:
  Controller();
  void begin(const config::DeviceConfig& configuration, uint32_t chipId);
  void update();
  void connect(const config::DeviceConfig& configuration);
  Mode mode() const { return mode_; }
  bool isProvisioning() const { return mode_ == Mode::Provisioning; }
  const char* ipAddress() const;
  const char* apSsid() const { return apSsid_; }

 private:
  void startProvisioning(uint32_t chipId);
  Mode mode_;
  uint32_t startedAt_;
  uint32_t chipId_;
  char apSsid_[25];
};

}  // namespace network
