#include "web/RequestValidation.h"

#include <string.h>

namespace web {

bool isBoundedText(const char* value, size_t maximumLength, bool required) {
  if (value == nullptr) return false;
  const size_t length = strnlen(value, maximumLength + 1);
  return length <= maximumLength && (!required || length > 0);
}

bool isDecimalInRange(const char* value, unsigned long maximum, unsigned long& parsed) {
  if (value == nullptr || *value == '\0') return false;
  unsigned long result = 0;
  for (const char* character = value; *character != '\0'; ++character) {
    if (*character < '0' || *character > '9') return false;
    const unsigned long digit = static_cast<unsigned long>(*character - '0');
    if (result > (maximum - digit) / 10) return false;
    result = result * 10 + digit;
  }
  parsed = result;
  return true;
}

bool isWellFormedUrlEncoding(const char* value) {
  if (value == nullptr) return false;
  for (const char* character = value; *character != '\0'; ++character) {
    if (*character != '%') continue;
    if (character[1] == '\0' || character[2] == '\0') return false;
    const char first = character[1];
    const char second = character[2];
    const bool firstIsHex = (first >= '0' && first <= '9') || (first >= 'A' && first <= 'F') ||
                            (first >= 'a' && first <= 'f');
    const bool secondIsHex = (second >= '0' && second <= '9') || (second >= 'A' && second <= 'F') ||
                             (second >= 'a' && second <= 'f');
    if (!firstIsHex || !secondIsHex) return false;
    character += 2;
  }
  return true;
}

NetworkFieldError validateNetworkFields(bool hasName, const char* name, bool hasSsid,
                                        const char* ssid, bool hasPassword,
                                        const char* password) {
  if (!hasName || name == nullptr || *name == '\0') return NetworkFieldError::MissingDeviceName;
  if (!isBoundedText(name, kMaximumNameLength, true)) return NetworkFieldError::InvalidDeviceName;
  if (!hasSsid || ssid == nullptr || *ssid == '\0') return NetworkFieldError::MissingWifiSsid;
  if (!isBoundedText(ssid, kMaximumSsidLength, true)) return NetworkFieldError::WifiSsidTooLong;
  if (!hasPassword || !isBoundedText(password, kMaximumPasswordLength, false))
    return NetworkFieldError::InvalidWifiPassword;
  return NetworkFieldError::None;
}

bool validNetworkFields(const char* name, const char* ssid, const char* password) {
  return validateNetworkFields(true, name, true, ssid, true, password) == NetworkFieldError::None;
}

bool validTallyFields(const char* state, const char* brightness, unsigned long& parsedBrightness) {
  return isBoundedText(state, 8, true) &&
         (brightness == nullptr || isDecimalInRange(brightness, 32, parsedBrightness));
}

size_t maximumUrlEncodedNetworkFormBytes() {
  // name= + &ssid= + &password=, with every accepted value byte encoded as
  // %XX. This remains valid for UTF-8 because field limits are byte limits.
  return 5 + 3 * kMaximumNameLength + 6 + 3 * kMaximumSsidLength +
         10 + 3 * kMaximumPasswordLength;
}

}  // namespace web
