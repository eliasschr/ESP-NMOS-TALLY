#pragma once

#include <stddef.h>

namespace web {

constexpr size_t kMaximumRequestBodyBytes = 256;
constexpr size_t kMaximumNameLength = 32;
constexpr size_t kMaximumSsidLength = 32;
constexpr size_t kMaximumPasswordLength = 64;

bool isBoundedText(const char* value, size_t maximumLength, bool required);
bool isDecimalInRange(const char* value, unsigned long maximum, unsigned long& parsed);
bool validNetworkFields(const char* name, const char* ssid, const char* password);
bool validTallyFields(const char* state, const char* brightness, unsigned long& parsedBrightness);

}  // namespace web
