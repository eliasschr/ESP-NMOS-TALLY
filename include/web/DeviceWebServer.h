#pragma once

#include "config/DeviceConfig.h"
#include "network/NetworkController.h"
#include "platform/Platform.h"
#include "tally/TallyState.h"
#include "web/DeferredNetworkTransition.h"

namespace web {

class Control {
 public:
  virtual ~Control() = default;
  virtual const config::DeviceConfig& configuration() const = 0;
  virtual const network::Controller& network() const = 0;
  virtual tally::State state() const = 0;
  virtual bool updateConfiguration(const config::DeviceConfig& value) = 0;
  // Called after the HTTP response accepting network settings has been sent.
  virtual void connectConfiguredNetwork() = 0;
  virtual bool setManualState(tally::State state) = 0;
};

class DeviceWebServer {
 public:
  explicit DeviceWebServer(Control& control);
  void begin();
  void handleClient();
  bool networkTransitionPending() const { return networkTransition_.pending(); }

 private:
  void serveIndex();
  void serveStatus();
  void saveNetwork();
  void setTally();
  bool authorizedStateChange();
  bool requestIsSameOrigin();
  bool requestBodyIsAcceptable();
  // Returns a stable, value-free diagnostic code, or nullptr for a valid form.
  const char* networkFormRejectionReason();
  void logNetworkFormRejection(const char* reason) const;
  void logNetworkFieldPresence(bool hasName, bool hasSsid, bool hasPassword) const;
  void logNetworkFieldValidation(bool hasName, bool validName, bool hasSsid,
                                 bool validSsid, bool hasPassword, bool validPassword) const;
  bool recordAuthorizationFailure();
  void sendError(int status, const char* message, const char* code = nullptr);

  Control& control_;
  TallyWebServer server_;
  uint8_t authorizationFailures_;
  uint32_t authorizationBlockedUntil_;
  DeferredNetworkTransition networkTransition_;
};

}  // namespace web
