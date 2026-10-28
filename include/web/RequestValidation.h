#pragma once

#include <stddef.h>

namespace web {

// application/x-www-form-urlencoded can expand each accepted input byte to
// three bytes. The three network fields require at most 405 bytes including
// their names and separators; 416 leaves a small fixed margin without making
// the post-parse ESP8266WebServer allocation unbounded.
constexpr size_t kMaximumRequestBodyBytes = 416;
constexpr size_t kMaximumNameLength = 32;
constexpr size_t kMaximumSsidLength = 32;
constexpr size_t kMaximumPasswordLength = 64;

enum class NetworkFieldError {
  None,
  MissingDeviceName,
  InvalidDeviceName,
  MissingWifiSsid,
  WifiSsidTooLong,
  InvalidWifiPassword,
};

bool isBoundedText(const char* value, size_t maximumLength, bool required);
bool isDecimalInRange(const char* value, unsigned long maximum, unsigned long& parsed);
bool isWellFormedUrlEncoding(const char* value);
NetworkFieldError validateNetworkFields(bool hasName, const char* name, bool hasSsid,
                                        const char* ssid, bool hasPassword,
                                        const char* password);
bool validNetworkFields(const char* name, const char* ssid, const char* password);
bool validTallyFields(const char* state, const char* brightness, unsigned long& parsedBrightness);
size_t maximumUrlEncodedNetworkFormBytes();

}  // namespace web
