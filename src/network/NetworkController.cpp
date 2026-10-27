#include "network/NetworkController.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <user_interface.h>

#include <string.h>

#include "security/ProvisioningAuth.h"

namespace network {
namespace {
constexpr uint32_t kProvisioningWindowMs = 10UL * 60UL * 1000UL;
}

Controller::Controller()
    : mode_(Mode::Unconfigured), connection_(), configuration_(), provisioningExpiresAt_(0),
      provisioningSecret_() {}

void Controller::begin(const config::DeviceConfig& configuration) {
  configuration_ = configuration;
  WiFi.persistent(false);
  const ConnectionAction action = connection_.begin(config::hasNetwork(configuration_), millis());
  if (action == ConnectionAction::Connect) startStationAttempt();
  else WiFi.mode(WIFI_OFF);
}

void Controller::connect(const config::DeviceConfig& configuration) {
  configuration_ = configuration;
  stopProvisioning();
  const ConnectionAction action = connection_.begin(config::hasNetwork(configuration_), millis());
  if (action == ConnectionAction::Connect) startStationAttempt();
}

void Controller::startStationAttempt() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(configuration_.wifiSsid, configuration_.wifiPassword);
  mode_ = Mode::Connecting;
}

void Controller::update() {
  if (isProvisioning()) {
    if (!security::provisioningWindowIsActive(millis(), provisioningExpiresAt_)) stopProvisioning();
    return;
  }
  const ConnectionAction action = connection_.update(WiFi.status() == WL_CONNECTED, millis());
  switch (connection_.state()) {
    case ConnectionState::Connected: mode_ = Mode::Station; break;
    case ConnectionState::Connecting: mode_ = Mode::Connecting; break;
    case ConnectionState::Backoff: mode_ = Mode::Recovering; break;
    case ConnectionState::Unconfigured: mode_ = Mode::Unconfigured; break;
  }
  if (action == ConnectionAction::Connect) startStationAttempt();
}

bool Controller::startProvisioning() {
  // os_random() is supplied by the ESP8266 SDK's RF-noise-backed PRNG. Do not
  // substitute Arduino random(), chip IDs, MAC addresses, or timestamps here.
  for (size_t index = 0; index < 12; ++index) {
    const uint8_t byte = static_cast<uint8_t>(os_random() & 0xFFU);
    snprintf(provisioningSecret_ + index * 2, 3, "%02X", byte);
  }
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(kProvisioningSsid, provisioningSecret_)) {
    clearProvisioningSecret();
    return false;
  }
  mode_ = Mode::Provisioning;
  provisioningExpiresAt_ = millis() + kProvisioningWindowMs;
  return true;
}

void Controller::stopProvisioning() {
  if (!isProvisioning()) return;
  WiFi.softAPdisconnect(true);
  clearProvisioningSecret();
  mode_ = Mode::Recovering;
  const ConnectionAction action = connection_.update(WiFi.status() == WL_CONNECTED, millis());
  if (connection_.state() == ConnectionState::Connected) mode_ = Mode::Station;
  if (connection_.state() == ConnectionState::Unconfigured) mode_ = Mode::Unconfigured;
  if (action == ConnectionAction::Connect) startStationAttempt();
}

void Controller::clearProvisioningSecret() {
  memset(provisioningSecret_, 0, sizeof(provisioningSecret_));
  provisioningExpiresAt_ = 0;
}

bool Controller::provisioningSecretMatches(const char* candidate) const {
  if (!isProvisioning() || candidate == nullptr) return false;
  return security::secretMatches(candidate, provisioningSecret_, sizeof(provisioningSecret_) - 1);
}

const char* Controller::ipAddress() const {
  static char address[16];
  const IPAddress ip = isProvisioning() ? WiFi.softAPIP() : WiFi.localIP();
  snprintf(address, sizeof(address), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return address;
}

}  // namespace network
