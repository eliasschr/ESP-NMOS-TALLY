#pragma once

#include <stdint.h>

#include "config/DeviceConfig.h"
#include "network/ConnectionState.h"

namespace network {

enum class Mode : uint8_t { Unconfigured, Connecting, Station, Provisioning, Recovering };

class Controller {
 public:
  Controller();
  void begin(const config::DeviceConfig& configuration);
  void update();
  void connect(const config::DeviceConfig& configuration);
  bool startProvisioning();
  Mode mode() const { return mode_; }
  bool isProvisioning() const { return mode_ == Mode::Provisioning; }
  bool isConnected() const { return mode_ == Mode::Station; }
  const char* ipAddress() const;
  const char* apSsid() const { return kProvisioningSsid; }
  bool provisioningSecretMatches(const char* candidate) const;
  const char* provisioningSecretForSerial() const { return provisioningSecret_; }

 private:
  void stopProvisioning();
  void startStationAttempt();
  void clearProvisioningSecret();
  static constexpr const char* kProvisioningSsid = "ESP-NMOS-TALLY-SETUP";
  Mode mode_;
  ConnectionStateMachine connection_;
  config::DeviceConfig configuration_;
  uint32_t provisioningExpiresAt_;
  char provisioningSecret_[25];
};

}  // namespace network
