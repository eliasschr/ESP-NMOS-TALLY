#include "web/DeviceWebServer.h"

#include <ESP8266WiFi.h>
#include <LittleFS.h>

namespace web {
namespace {
constexpr size_t kMaxFormFieldLength = 64;

String jsonEscape(const char* value) {
  String result;
  while (*value) {
    if (*value == '"' || *value == '\\') result += '\\';
    result += *value++;
  }
  return result;
}
}

DeviceWebServer::DeviceWebServer(Control& control) : control_(control), server_(80) {}

void DeviceWebServer::begin() {
  setupToken_ = String(ESP.getChipId() ^ micros(), HEX) + String(ESP.getCycleCount(), HEX);
  server_.collectHeaders("X-Setup-Token");
  server_.on("/", HTTP_GET, [this] { serveIndex(); });
  server_.on("/api/status", HTTP_GET, [this] { serveStatus(); });
  server_.on("/api/setup", HTTP_GET, [this] { serveSetup(); });
  server_.on("/api/network", HTTP_POST, [this] { saveNetwork(); });
  server_.on("/api/tally", HTTP_POST, [this] { setTally(); });
  server_.serveStatic("/app.css", LittleFS, "/www/app.css");
  server_.serveStatic("/app.js", LittleFS, "/www/app.js");
  server_.onNotFound([this] { sendError(404, "Not found"); });
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
  String body = "{\"deviceName\":\"" + jsonEscape(configuration.deviceName) + "\",\"platform\":\"ESP8266\",\"uptimeSeconds\":" + String(millis() / 1000) +
                ",\"freeHeap\":" + String(ESP.getFreeHeap()) + ",\"wifiMode\":\"" +
                (control_.network().isProvisioning() ? "provisioning" : "station") + "\",\"ipAddress\":\"" +
                control_.network().ipAddress() + "\",\"tallyState\":\"" + tally::StateMachine::name(control_.state()) +
                "\",\"source\":\"manual\",\"nmos\":\"planned\"}";
  server_.send(200, "application/json", body);
}

void DeviceWebServer::serveSetup() {
  if (!control_.network().isProvisioning()) return sendError(403, "Setup is available only in provisioning mode.");
  server_.send(200, "application/json", "{\"token\":\"" + setupToken_ + "\",\"apSsid\":\"" +
      jsonEscape(control_.network().apSsid()) + "\"}");
}

bool DeviceWebServer::setupRequestAllowed() const {
  return control_.network().isProvisioning() && server_.hasHeader("X-Setup-Token") &&
         server_.header("X-Setup-Token") == setupToken_;
}

void DeviceWebServer::saveNetwork() {
  if (!setupRequestAllowed()) return sendError(403, "Provisioning authorization is required.");
  const String name = server_.arg("name");
  const String ssid = server_.arg("ssid");
  const String password = server_.arg("password");
  if (name.length() == 0 || name.length() > kMaxFormFieldLength || ssid.length() == 0 ||
      ssid.length() > 32 || password.length() > 64) return sendError(400, "Invalid network configuration.");
  config::DeviceConfig next = control_.configuration();
  if (!config::copyBounded(next.deviceName, sizeof(next.deviceName), name.c_str()) ||
      !config::copyBounded(next.wifiSsid, sizeof(next.wifiSsid), ssid.c_str()) ||
      !config::copyBounded(next.wifiPassword, sizeof(next.wifiPassword), password.c_str()) ||
      !control_.updateConfiguration(next)) return sendError(500, "Could not save configuration.");
  server_.send(200, "application/json", "{\"status\":\"saved\"}");
}

void DeviceWebServer::setTally() {
  if (!setupRequestAllowed()) return sendError(403, "Manual tests are available only in provisioning mode.");
  tally::State state;
  if (!tally::StateMachine::parse(server_.arg("state").c_str(), state)) return sendError(400, "Invalid tally state.");
  if (server_.hasArg("brightness")) {
    const long brightness = server_.arg("brightness").toInt();
    if (brightness < 0 || brightness > 255) return sendError(400, "Invalid brightness.");
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
