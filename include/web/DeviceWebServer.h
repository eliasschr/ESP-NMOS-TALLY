#pragma once

#include <ESP8266WebServer.h>

#include "config/DeviceConfig.h"
#include "network/NetworkController.h"
#include "tally/TallyState.h"

namespace web {

class Control {
 public:
  virtual ~Control() = default;
  virtual const config::DeviceConfig& configuration() const = 0;
  virtual const network::Controller& network() const = 0;
  virtual tally::State state() const = 0;
  virtual bool updateConfiguration(const config::DeviceConfig& value) = 0;
  virtual bool setManualState(tally::State state) = 0;
};

class DeviceWebServer {
 public:
  explicit DeviceWebServer(Control& control);
  void begin();
  void handleClient();

 private:
  void serveIndex();
  void serveStatus();
  void saveNetwork();
  void setTally();
  bool authorizedStateChange();
  bool requestIsSameOrigin() const;
  bool requestBodyIsAcceptable() const;
  bool recordAuthorizationFailure();
  void sendError(int status, const char* message);

  Control& control_;
  ESP8266WebServer server_;
  uint8_t authorizationFailures_;
  uint32_t authorizationBlockedUntil_;
};

}  // namespace web
