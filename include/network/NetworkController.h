#pragma once

#include <stdint.h>

#include "config/DeviceConfig.h"
#include "network/ConnectionState.h"
#include "platform/Platform.h"

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
  bool hasConfiguration() const;
  bool hasStationIp() const;
  const char* wifiState() const;
  const char* stationStatusName() const;
  int stationStatusCode() const;
  uint16_t connectionAttemptCount() const { return connection_.attemptCount(); }
  int lastDisconnectReason() const { return lastDisconnectReason_; }
  const char* lastDisconnectCategory() const;
  const char* ipAddress() const;
  const char* apSsid() const { return kProvisioningSsid; }
  bool provisioningSecretMatches(const char* candidate) const;
  const char* provisioningSecretForSerial() const { return provisioningSecret_; }

 private:
  void stopProvisioning();
  void startStationAttempt();
  void registerWiFiEvents();
  void logConnectionAction(ConnectionState previousState, ConnectionAction action);
  void onStationAssociated();
  void onStationDisconnected(int reason);
  void onStationGotIp();
  void clearProvisioningSecret();
  static constexpr const char* kProvisioningSsid = "ESP-NMOS-TALLY-SETUP";
  Mode mode_;
  ConnectionStateMachine connection_;
  config::DeviceConfig configuration_;
  uint32_t provisioningExpiresAt_;
  char provisioningSecret_[25];
  int lastDisconnectReason_;
#if defined(ESP8266)
  WiFiEventHandler stationAssociatedHandler_;
  WiFiEventHandler stationDisconnectedHandler_;
  WiFiEventHandler stationGotIpHandler_;
#elif defined(ESP32)
  WiFiEventId_t stationAssociatedHandler_;
  WiFiEventId_t stationDisconnectedHandler_;
  WiFiEventId_t stationGotIpHandler_;
#endif
};

}  // namespace network
