#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config/DeviceConfig.h"
#include "tally/TallyState.h"

namespace serial {

constexpr size_t kMaximumCommandLength = 31;

enum class InputEvent : uint8_t { None, LineReady, Oversized };

// Collects one command without allocating memory or waiting for more input.
class InputBuffer {
 public:
  InputBuffer();

  InputEvent append(char character);
  const char* line() const { return line_; }

 private:
  char line_[kMaximumCommandLength + 1];
  size_t length_;
  bool discardingOversizedLine_;
};

enum class CommandType : uint8_t {
  Empty,
  Invalid,
  Help,
  Status,
  Provision,
  EnterDiagnostics,
  ExitDiagnostics,
  SetTally,
  LedTest,
  LedRainbow,
  LedStop,
  LedBrightness,
  LedBrightnessStatus,
  WifiStatus,
};

struct Command {
  CommandType type;
  tally::State state;
  uint8_t brightness;
};

Command parseCommand(const char* line);

struct LedTestOutput {
  const char* name;
  tally::RgbColor color;
};

// Keeps diagnostic authorization and LED-test timing independent from Arduino
// serial and hardware APIs so their behavior can be verified on the host.
class DiagnosticSession {
 public:
  DiagnosticSession();

  bool active() const { return active_; }
  bool ledTestActive() const { return ledTestActive_; }
  bool rainbowActive() const { return rainbowActive_; }
  uint8_t brightness() const { return active_ ? diagnosticBrightness_ : configuredBrightness_; }
  bool enter(tally::State returnState, uint8_t configuredBrightness);
  bool exit(tally::State& restoredState);
  bool setBrightness(uint8_t brightness);
  bool startLedTest(uint32_t now, tally::State returnState);
  bool cancelLedTest(tally::State& restoredState);
  bool startRainbow(tally::State returnState);
  bool stopRainbow(tally::State& restoredState);
  bool cancelEffects(tally::State& restoredState);
  // Returns true only when the next output must be rendered. Completion
  // restores the pre-test tally state and also returns true.
  bool updateLedTest(uint32_t now, tally::State& restoredState);
  LedTestOutput ledTestOutput() const;

 private:
  static constexpr uint32_t kLedTestStepMs = 1000;

  bool active_;
  bool ledTestActive_;
  bool rainbowActive_;
  uint8_t configuredBrightness_;
  uint8_t diagnosticBrightness_;
  uint8_t ledTestStep_;
  uint32_t nextLedTestTransitionAt_;
  tally::State returnState_;
  tally::State preTestState_;
};

}  // namespace serial
