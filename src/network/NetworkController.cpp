#include "network/NetworkController.h"

#include <Arduino.h>

#include <string.h>

#include "platform/Platform.h"
#include "security/ProvisioningAuth.h"

namespace network {
namespace {
constexpr uint32_t kProvisioningWindowMs = 10UL * 60UL * 1000UL;

bool hasUsableAddress(const IPAddress& address) {
  return address[0] != 0 || address[1] != 0 || address[2] != 0 || address[3] != 0;
}

const char* statusName(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "WL_UNKNOWN";
  }
}

const char* disconnectCategory(int reason) {
  // Reason values differ between ESP8266 SDK and ESP32 IDF. Keep the numeric
  // value for diagnosis and use only broadly safe categories here.
  switch (reason) {
    case 2: return "auth-expired";
    case 4: return "association-failed";
    case 15: return "four-way-handshake-timeout";
    case 201: return "no-ap-found";
    case 202: return "auth-failed";
    default: return "other";
  }
}
}

Controller::Controller()
    : mode_(Mode::Unconfigured), connection_(), configuration_(), provisioningExpiresAt_(0),
      provisioningSecret_(), lastDisconnectReason_(-1) {}

void Controller::begin(const config::DeviceConfig& configuration) {
  configuration_ = configuration;
  platform::prepareWiFi();
  registerWiFiEvents();
  const ConnectionAction action = connection_.begin(config::hasNetwork(configuration_), millis());
  if (action == ConnectionAction::Connect) startStationAttempt();
  else {
    // WebServer opens its listener during application startup. On ESP32 the
    // TCP/IP task is created with the STA interface; selecting WIFI_OFF from
    // the initial null mode leaves that task uninitialized.
    // This starts neither a station connection nor a provisioning AP.
    WiFi.mode(WIFI_STA);
  }
}

void Controller::connect(const config::DeviceConfig& configuration) {
  configuration_ = configuration;
  lastDisconnectReason_ = -1;
  Serial.println(F("INFO: WIFI: configuration saved"));
  stopProvisioning();
  const ConnectionAction action = connection_.begin(config::hasNetwork(configuration_), millis());
  if (action == ConnectionAction::Connect) startStationAttempt();
}

void Controller::startStationAttempt() {
  if (!config::hasNetwork(configuration_)) {
    Serial.println(F("WARN: WIFI: station connection skipped; configuration is missing SSID"));
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(configuration_.wifiSsid, configuration_.wifiPassword);
  mode_ = Mode::Connecting;
  Serial.print(F("INFO: WIFI: station connection requested; attempt="));
  Serial.println(connection_.attemptCount());
}

void Controller::update() {
  if (isProvisioning()) {
    if (!security::provisioningWindowIsActive(millis(), provisioningExpiresAt_)) stopProvisioning();
    return;
  }
  const bool connected = WiFi.status() == WL_CONNECTED && hasStationIp();
  const ConnectionState previousState = connection_.state();
  const ConnectionAction action = connection_.update(connected, millis());
  switch (connection_.state()) {
    case ConnectionState::Connected: mode_ = Mode::Station; break;
    case ConnectionState::Connecting: mode_ = Mode::Connecting; break;
    case ConnectionState::Backoff: mode_ = Mode::Recovering; break;
    case ConnectionState::Unconfigured: mode_ = Mode::Unconfigured; break;
  }
  if (previousState != ConnectionState::Connected && connection_.state() == ConnectionState::Connected)
    Serial.println(F("INFO: WIFI: connected"));
  logConnectionAction(previousState, action);
  if (action == ConnectionAction::Connect) startStationAttempt();
}

bool Controller::startProvisioning() {
  // platform::secureRandom() selects the hardware/SDK entropy source for the
  // active target. Do not substitute Arduino random(), chip IDs, or timestamps.
  for (size_t index = 0; index < 12; ++index) {
    const uint8_t byte = static_cast<uint8_t>(platform::secureRandom() & 0xFFU);
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
  Serial.println(F("INFO: WIFI: AP shutdown requested"));
  WiFi.softAPdisconnect(true);
  clearProvisioningSecret();
  mode_ = Mode::Recovering;
  const ConnectionAction action = connection_.update(WiFi.status() == WL_CONNECTED && hasStationIp(), millis());
  if (connection_.state() == ConnectionState::Connected) mode_ = Mode::Station;
  if (connection_.state() == ConnectionState::Unconfigured) mode_ = Mode::Unconfigured;
  logConnectionAction(ConnectionState::Connecting, action);
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

bool Controller::hasConfiguration() const { return config::hasNetwork(configuration_); }

bool Controller::hasStationIp() const { return hasUsableAddress(WiFi.localIP()); }

const char* Controller::wifiState() const {
  if (!hasConfiguration()) return "unconfigured";
  if (isProvisioning()) return "provisioning";
  if (connection_.state() == ConnectionState::Connected &&
      WiFi.status() == WL_CONNECTED && hasStationIp()) return "connected";
  if (connection_.state() == ConnectionState::Connecting) return "connecting";
  if (connection_.state() == ConnectionState::Backoff) return "cooling_down";
  return "failed";
}

const char* Controller::stationStatusName() const { return statusName(WiFi.status()); }

int Controller::stationStatusCode() const { return static_cast<int>(WiFi.status()); }

const char* Controller::lastDisconnectCategory() const {
  return lastDisconnectReason_ < 0 ? "none" : disconnectCategory(lastDisconnectReason_);
}

void Controller::logConnectionAction(ConnectionState previousState, ConnectionAction action) {
  if (previousState == ConnectionState::Connecting &&
      connection_.state() == ConnectionState::Backoff && action == ConnectionAction::None) {
    Serial.print(F("WARN: WIFI: connection timeout; retry scheduled at="));
    Serial.println(connection_.nextAttemptAt());
  }
}

void Controller::onStationAssociated() { Serial.println(F("INFO: WIFI: station associated")); }

void Controller::onStationDisconnected(int reason) {
  lastDisconnectReason_ = reason;
  Serial.print(F("WARN: WIFI: station disconnected, reason="));
  Serial.print(reason);
  Serial.print(F(", category="));
  Serial.println(lastDisconnectCategory());
}

void Controller::onStationGotIp() { Serial.println(F("INFO: WIFI: DHCP completed")); }

void Controller::registerWiFiEvents() {
#if defined(ESP8266)
  stationAssociatedHandler_ = WiFi.onStationModeConnected(
      [this](const WiFiEventStationModeConnected&) { onStationAssociated(); });
  stationDisconnectedHandler_ = WiFi.onStationModeDisconnected(
      [this](const WiFiEventStationModeDisconnected& event) { onStationDisconnected(event.reason); });
  stationGotIpHandler_ = WiFi.onStationModeGotIP(
      [this](const WiFiEventStationModeGotIP&) { onStationGotIp(); });
#elif defined(ESP32)
  stationAssociatedHandler_ = WiFi.onEvent(
      [this](WiFiEvent_t, WiFiEventInfo_t) { onStationAssociated(); }, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  stationDisconnectedHandler_ = WiFi.onEvent(
      [this](WiFiEvent_t, WiFiEventInfo_t event) { onStationDisconnected(event.wifi_sta_disconnected.reason); },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  stationGotIpHandler_ = WiFi.onEvent(
      [this](WiFiEvent_t, WiFiEventInfo_t) { onStationGotIp(); }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
#endif
}

const char* Controller::ipAddress() const {
  static char address[16];
  const IPAddress ip = isProvisioning() ? WiFi.softAPIP() : WiFi.localIP();
  snprintf(address, sizeof(address), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return address;
}

}  // namespace network
