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

bool validNetworkFields(const char* name, const char* ssid, const char* password) {
  return isBoundedText(name, kMaximumNameLength, true) &&
         isBoundedText(ssid, kMaximumSsidLength, true) &&
         isBoundedText(password, kMaximumPasswordLength, false);
}

bool validTallyFields(const char* state, const char* brightness, unsigned long& parsedBrightness) {
  return isBoundedText(state, 8, true) &&
         (brightness == nullptr || isDecimalInRange(brightness, 32, parsedBrightness));
}

}  // namespace web
