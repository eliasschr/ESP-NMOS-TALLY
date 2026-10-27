#include "network/NetworkController.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>

namespace network {
namespace {
constexpr uint32_t kConnectTimeoutMs = 20000;
}

Controller::Controller() : mode_(Mode::Provisioning), startedAt_(0), chipId_(0), apSsid_() {}

void Controller::begin(const config::DeviceConfig& configuration, uint32_t chipId) {
  chipId_ = chipId;
  WiFi.persistent(false);
  if (config::hasNetwork(configuration)) {
    connect(configuration);
    return;
  }
  startProvisioning(chipId_);
}

void Controller::connect(const config::DeviceConfig& configuration) {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(configuration.wifiSsid, configuration.wifiPassword);
  mode_ = Mode::Connecting;
  startedAt_ = millis();
}

void Controller::update() {
  if (mode_ != Mode::Connecting) return;
  if (WiFi.status() == WL_CONNECTED) {
    mode_ = Mode::Station;
    return;
  }
  if (millis() - startedAt_ >= kConnectTimeoutMs) startProvisioning(chipId_);
}

void Controller::startProvisioning(uint32_t chipId) {
  // A per-device passphrase avoids a shared default. It is not strong against a
  // nearby attacker who can infer the public chip ID; see docs/security.md.
  char password[16];
  snprintf(apSsid_, sizeof(apSsid_), "ESP-NMOS-TALLY-%06lX", static_cast<unsigned long>(chipId & 0xFFFFFFUL));
  snprintf(password, sizeof(password), "Tally-%06lX",
           static_cast<unsigned long>((chipId ^ 0x5A3C19UL) & 0xFFFFFFUL));
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid_, password);
  mode_ = Mode::Provisioning;
}

const char* Controller::ipAddress() const {
  static char address[16];
  const IPAddress ip = mode_ == Mode::Station ? WiFi.localIP() : WiFi.softAPIP();
  snprintf(address, sizeof(address), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return address;
}

}  // namespace network
