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
  const char* headers[] = {"X-Provisioning-Secret", "Origin", "Host", "Content-Length"};
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

void DeviceWebServer::handleClient() { server_.handleClient(); }

void DeviceWebServer::serveIndex() {
  File page = LittleFS.open("/www/index.html", "r");
  if (!page) {
    sendError(503, "Web resources are unavailable. Upload the LittleFS image.");
    return;
  }
  server_.streamFile(page, "text/html");
  page.close();
}

void DeviceWebServer::serveStatus() {
  const auto& configuration = control_.configuration();
  String body = "{\"deviceName\":\"" + jsonEscape(configuration.deviceName) + "\",\"platform\":\"" + platform::name() + "\",\"uptimeSeconds\":" + String(millis() / 1000) +
                ",\"freeHeap\":" + String(platform::freeHeap()) + ",\"wifiMode\":\"" +
                (control_.network().isProvisioning() ? "provisioning" :
                 (control_.network().isConnected() ? "station" : "recovering")) + "\",\"ipAddress\":\"" +
                control_.network().ipAddress() + "\",\"tallyState\":\"" + tally::StateMachine::name(control_.state()) +
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
  if (!control_.network().isProvisioning() || !requestIsSameOrigin() ||
      !server_.hasHeader("X-Provisioning-Secret") ||
      !control_.network().provisioningSecretMatches(server_.header("X-Provisioning-Secret").c_str())) {
    recordAuthorizationFailure();
    sendError(403, "Physical provisioning authorization is required.");
    return false;
  }
  authorizationFailures_ = 0;
  return true;
}

void DeviceWebServer::saveNetwork() {
  if (!authorizedStateChange()) return;
  if (!requestBodyIsAcceptable()) return sendError(413, "Request body is too large.");
  const String name = server_.arg("name");
  const String ssid = server_.arg("ssid");
  const String password = server_.arg("password");
  if (!validNetworkFields(name.c_str(), ssid.c_str(), password.c_str()))
    return sendError(400, "Invalid network configuration.");
  config::DeviceConfig next = control_.configuration();
  if (!config::copyBounded(next.deviceName, sizeof(next.deviceName), name.c_str()) ||
      !config::copyBounded(next.wifiSsid, sizeof(next.wifiSsid), ssid.c_str()) ||
      !config::copyBounded(next.wifiPassword, sizeof(next.wifiPassword), password.c_str()) ||
      !control_.updateConfiguration(next)) return sendError(500, "Could not save configuration.");
  server_.send(200, "application/json", "{\"status\":\"saved\"}");
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

void DeviceWebServer::sendError(int status, const char* message) {
  server_.send(status, "application/json", String("{\"error\":\"") + message + "\"}");
}

}  // namespace web
