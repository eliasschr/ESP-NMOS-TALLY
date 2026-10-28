#include "web/DeviceWebServer.h"

#include <LittleFS.h>

#include "platform/Platform.h"
#include "web/RequestValidation.h"

namespace web {
namespace {
constexpr uint8_t kMaximumAuthorizationFailures = 5;
constexpr uint32_t kAuthorizationCooldownMs = 30000;

String jsonEscape(const char* value) {
  String result;
  while (*value) {
    if (*value == '"' || *value == '\\') result += '\\';
    result += *value++;
  }
  return result;
}
}

DeviceWebServer::DeviceWebServer(Control& control)
    : control_(control), server_(80), authorizationFailures_(0), authorizationBlockedUntil_(0) {}

void DeviceWebServer::begin() {
  const char* headers[] = {"X-Provisioning-Secret", "Origin", "Host", "Content-Length", "Content-Type"};
  server_.collectHeaders(headers, sizeof(headers) / sizeof(headers[0]));
  server_.on("/", HTTP_GET, [this] { serveIndex(); });
  server_.on("/api/status", HTTP_GET, [this] { serveStatus(); });
  server_.on("/api/network", HTTP_POST, [this] { saveNetwork(); });
  server_.on("/api/tally", HTTP_POST, [this] { setTally(); });
  server_.serveStatic("/app.css", LittleFS, "/www/app.css");
  server_.serveStatic("/app.js", LittleFS, "/www/app.js");
  server_.onNotFound([this] {
    if (server_.uri() == "/api/network" || server_.uri() == "/api/tally") sendError(405, "Method not allowed");
    else sendError(404, "Not found");
  });
  server_.begin();
}

void DeviceWebServer::handleClient() {
  server_.handleClient();
  if (!networkTransition_.consume()) return;
  // Finish the response while the setup AP is still available before switching.
  control_.connectConfiguredNetwork();
}

void DeviceWebServer::serveIndex() {
  File page = LittleFS.open("/www/index.html", "r");
  if (!page) {
    sendError(503, "Web resources are unavailable. Upload the LittleFS image.");
    return;
  }
  // The setup page selects versioned assets. Do not let a browser retain an
  // older HTML entry point after a provisioning-dashboard update.
  server_.sendHeader("Cache-Control", "no-store, max-age=0");
  server_.streamFile(page, "text/html");
  page.close();
}

void DeviceWebServer::serveStatus() {
  const auto& configuration = control_.configuration();
  String body = "{\"deviceName\":\"" + jsonEscape(configuration.deviceName) + "\",\"platform\":\"" + platform::name() + "\",\"uptimeSeconds\":" + String(millis() / 1000) +
                ",\"freeHeap\":" + String(platform::freeHeap()) + ",\"wifiMode\":\"" +
                (control_.network().isProvisioning() ? "provisioning" :
                 (control_.network().isConnected() ? "station" : "recovering")) + "\",\"ipAddress\":\"" +
                control_.network().ipAddress() + "\",\"wifiState\":\"" + control_.network().wifiState() +
                "\",\"tallyState\":\"" + tally::StateMachine::name(control_.state()) +
                "\",\"source\":\"manual\",\"nmos\":\"planned\"}";
  server_.send(200, "application/json", body);
}

bool DeviceWebServer::requestIsSameOrigin() {
  if (!server_.hasHeader("Origin")) return true;
  if (!server_.hasHeader("Host")) return false;
  return server_.header("Origin") == String("http://") + server_.header("Host");
}

bool DeviceWebServer::requestBodyIsAcceptable() {
  // ESP8266WebServer parses form data before route handlers run, so this is a
  // post-parse rejection and cannot promise protection from parser allocation.
  unsigned long contentLength = 0;
  return server_.hasHeader("Content-Length") &&
         isDecimalInRange(server_.header("Content-Length").c_str(),
                          kMaximumRequestBodyBytes, contentLength);
}

const char* DeviceWebServer::networkFormRejectionReason() {
  if (!server_.hasHeader("Content-Type") ||
      !server_.header("Content-Type").startsWith("application/x-www-form-urlencoded"))
    return "malformed-content-type";

  // ESP8266WebServer adds the raw body as an internal `plain` argument but
  // excludes it from args(). ESP32 WebServer includes it, so ignore only its
  // final internal argument; a submitted field named `plain` still fails.
  int fieldCount = server_.args();
#if defined(ESP32)
  if (fieldCount == 0 || server_.argName(fieldCount - 1) != "plain")
    return "invalid-form-encoding";
  --fieldCount;
#endif

  bool name = false;
  bool ssid = false;
  bool password = false;
  for (int index = 0; index < fieldCount; ++index) {
    const String field = server_.argName(index);
    if (field == "name") {
      if (name) return "duplicate-field";
      name = true;
    } else if (field == "ssid") {
      if (ssid) return "duplicate-field";
      ssid = true;
    } else if (field == "password") {
      if (password) return "duplicate-field";
      password = true;
    } else return "unexpected-form-field";
  }
  if (!name) return "missing-name-field";
  if (!ssid) return "missing-ssid-field";
  if (!password) return "missing-password-field";

  // The framework's decoder accepts malformed percent escapes. Validate its
  // internal raw-body argument without recording or returning that body.
  if (!isWellFormedUrlEncoding(server_.arg("plain").c_str()))
    return "invalid-form-encoding";
  return nullptr;
}

void DeviceWebServer::logNetworkFormRejection(const char* reason) const {
  Serial.print(F("WARN: HTTP network form rejected: "));
  Serial.println(reason);
}

void DeviceWebServer::logNetworkFieldPresence(bool hasName, bool hasSsid,
                                              bool hasPassword) const {
  Serial.print(F("INFO: HTTP network form fields: name="));
  Serial.print(hasName ? F("present") : F("missing"));
  Serial.print(F(", ssid="));
  Serial.print(hasSsid ? F("present") : F("missing"));
  Serial.print(F(", password="));
  Serial.println(hasPassword ? F("present") : F("missing"));
}

void DeviceWebServer::logNetworkFieldValidation(bool hasName, bool validName, bool hasSsid,
                                                bool validSsid, bool hasPassword,
                                                bool validPassword) const {
  Serial.print(F("INFO: HTTP network form: name="));
  Serial.print(hasName ? (validName ? F("present-valid") : F("present-invalid")) : F("missing"));
  Serial.print(F(", ssid="));
  Serial.print(hasSsid ? (validSsid ? F("present-valid") : F("present-invalid")) : F("missing"));
  Serial.print(F(", password="));
  Serial.println(hasPassword ? (validPassword ? F("present-valid") : F("present-invalid")) : F("missing"));
}

bool DeviceWebServer::recordAuthorizationFailure() {
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - authorizationBlockedUntil_) < 0) return false;
  if (++authorizationFailures_ < kMaximumAuthorizationFailures) return true;
  authorizationFailures_ = 0;
  authorizationBlockedUntil_ = now + kAuthorizationCooldownMs;
  return false;
}

bool DeviceWebServer::authorizedStateChange() {
  if (static_cast<int32_t>(millis() - authorizationBlockedUntil_) < 0) {
    sendError(429, "Too many authorization failures. Try again later.");
    return false;
  }
  if (!control_.network().isProvisioning()) {
    sendError(403, "Provisioning is no longer active.", "provisioning_inactive");
    return false;
  }
  if (!requestIsSameOrigin() ||
      !server_.hasHeader("X-Provisioning-Secret") ||
      !control_.network().provisioningSecretMatches(server_.header("X-Provisioning-Secret").c_str())) {
    if (!recordAuthorizationFailure()) {
      sendError(429, "Too many authorization failures. Try again later.");
      return false;
    }
    sendError(403, "Physical provisioning authorization is required.");
    return false;
  }
  authorizationFailures_ = 0;
  return true;
}

void DeviceWebServer::saveNetwork() {
  if (!authorizedStateChange()) return;
  if (!requestBodyIsAcceptable()) {
    const char* reason = !server_.hasHeader("Content-Length") ? "missing-content-length" :
                         "invalid-or-oversized-content-length";
    logNetworkFormRejection(reason);
    return sendError(413, "Request body is too large.");
  }
  const char* formRejectionReason = networkFormRejectionReason();
  if (formRejectionReason != nullptr) {
    // Presence flags distinguish a browser serialization failure from an empty
    // or invalid submitted value without exposing credentials on serial.
    logNetworkFieldPresence(server_.hasArg("name"), server_.hasArg("ssid"),
                            server_.hasArg("password"));
    logNetworkFormRejection(formRejectionReason);
    return sendError(400, "Malformed form submission.", "malformed_form");
  }
  const bool hasName = server_.hasArg("name");
  const bool hasSsid = server_.hasArg("ssid");
  const bool hasPassword = server_.hasArg("password");
  const String name = server_.arg("name");
  const String ssid = server_.arg("ssid");
  const String password = server_.arg("password");
  const NetworkFieldError validation = validateNetworkFields(hasName, name.c_str(), hasSsid, ssid.c_str(),
                                                             hasPassword, password.c_str());
  logNetworkFieldValidation(hasName, isBoundedText(name.c_str(), kMaximumNameLength, true), hasSsid,
                            isBoundedText(ssid.c_str(), kMaximumSsidLength, true), hasPassword,
                            isBoundedText(password.c_str(), kMaximumPasswordLength, false));
  switch (validation) {
    case NetworkFieldError::None: break;
    case NetworkFieldError::MissingDeviceName:
      return sendError(400, "Missing device name.", "missing_device_name");
    case NetworkFieldError::InvalidDeviceName:
      return sendError(400, "Invalid device name.", "invalid_device_name");
    case NetworkFieldError::MissingWifiSsid:
      return sendError(400, "Missing Wi-Fi SSID.", "missing_wifi_ssid");
    case NetworkFieldError::WifiSsidTooLong:
      return sendError(400, "Wi-Fi SSID is too long.", "wifi_ssid_too_long");
    case NetworkFieldError::InvalidWifiPassword:
      return sendError(400, "Wi-Fi password is too long or otherwise invalid.", "invalid_wifi_password");
  }
  config::DeviceConfig next = control_.configuration();
  if (!config::copyBounded(next.deviceName, sizeof(next.deviceName), name.c_str()) ||
      !config::copyBounded(next.wifiSsid, sizeof(next.wifiSsid), ssid.c_str()) ||
      !config::copyBounded(next.wifiPassword, sizeof(next.wifiPassword), password.c_str()) ||
      !control_.updateConfiguration(next)) return sendError(500, "Could not save configuration.");
  server_.send(200, "application/json", "{\"status\":\"saved\"}");
  networkTransition_.schedule();
}

void DeviceWebServer::setTally() {
  if (!authorizedStateChange()) return;
  if (!requestBodyIsAcceptable()) return sendError(413, "Request body is too large.");
  tally::State state;
  const String requestedState = server_.arg("state");
  const String requestedBrightness = server_.arg("brightness");
  unsigned long brightness = 0;
  if (!validTallyFields(requestedState.c_str(), server_.hasArg("brightness") ? requestedBrightness.c_str() : nullptr,
                        brightness) || !tally::StateMachine::parse(requestedState.c_str(), state))
    return sendError(400, "Invalid tally state.");
  if (server_.hasArg("brightness")) {
    config::DeviceConfig next = control_.configuration();
    next.brightness = static_cast<uint8_t>(brightness);
    if (!control_.updateConfiguration(next)) return sendError(500, "Could not save brightness.");
  }
  control_.setManualState(state);
  server_.send(200, "application/json", "{\"status\":\"updated\"}");
}

void DeviceWebServer::sendError(int status, const char* message, const char* code) {
  String body = String("{\"error\":\"") + message + "\"";
  if (code != nullptr) body += String(",\"code\":\"") + code + "\"";
  server_.send(status, "application/json", body + "}");
}

}  // namespace web
