#include "security/ProvisioningAuth.h"

#include <string.h>

namespace security {

bool secretMatches(const char* candidate, const char* expected, size_t expectedLength) {
  if (candidate == nullptr || expected == nullptr || strnlen(candidate, expectedLength + 1) != expectedLength) return false;
  uint8_t difference = 0;
  for (size_t index = 0; index < expectedLength; ++index) difference |= candidate[index] ^ expected[index];
  return difference == 0;
}

bool provisioningWindowIsActive(uint32_t now, uint32_t expiresAt) {
  return expiresAt != 0 && static_cast<int32_t>(now - expiresAt) < 0;
}

}  // namespace security
