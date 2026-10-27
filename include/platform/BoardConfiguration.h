#pragma once

#include <stdint.h>

namespace platform {

enum class BoardTarget : uint8_t { Esp8266Feather, Esp32DevKit };

constexpr uint8_t kEsp8266FeatherLedPin = 15;
constexpr uint8_t kEsp32DevKitLedPin = 18;

// This stays Arduino-independent so board-pin policy remains unit-testable.
constexpr bool isLedPinValidForTarget(BoardTarget target, uint8_t pin) {
  return (target == BoardTarget::Esp8266Feather && pin == kEsp8266FeatherLedPin) ||
         (target == BoardTarget::Esp32DevKit && pin == kEsp32DevKitLedPin);
}

}  // namespace platform
