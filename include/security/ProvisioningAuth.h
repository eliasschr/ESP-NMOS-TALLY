#pragma once

#include <stddef.h>
#include <stdint.h>

namespace security {

bool secretMatches(const char* candidate, const char* expected, size_t expectedLength);
bool provisioningWindowIsActive(uint32_t now, uint32_t expiresAt);

}  // namespace security
