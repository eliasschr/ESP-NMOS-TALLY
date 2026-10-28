#include "serial/SerialDiagnostics.h"

#include <string.h>

namespace serial {
namespace {

bool isSpace(char value) { return value == ' ' || value == '\t'; }

char upper(char value) {
  return value >= 'a' && value <= 'z' ? static_cast<char>(value - ('a' - 'A')) : value;
}

const char* skipSpaces(const char* value) {
  while (*value != '\0' && isSpace(*value)) ++value;
  return value;
}

bool readToken(const char*& value, const char* expected) {
  while (*expected != '\0') {
    if (*value == '\0' || upper(*value) != *expected) return false;
    ++value;
    ++expected;
  }
  return *value == '\0' || isSpace(*value);
}

bool hasNoMoreTokens(const char* value) { return *skipSpaces(value) == '\0'; }

bool readBrightness(const char* value, uint8_t& brightness) {
  uint8_t parsed = 0;
  if (*value == '\0') return false;
  while (*value != '\0') {
    if (*value < '0' || *value > '9') return false;
    const uint8_t digit = static_cast<uint8_t>(*value - '0');
    // Checking before multiplication avoids overflow and rejects values over
    // the physical brightness limit rather than silently clamping them.
    if (parsed > (config::kMaximumBrightness - digit) / 10U) return false;
    parsed = static_cast<uint8_t>(parsed * 10U + digit);
    ++value;
  }
  brightness = parsed;
  return true;
}

const LedTestOutput kLedTestOutputs[] = {
    {"RED", {255, 0, 0}},     {"GREEN", {0, 255, 0}},
    {"BLUE", {0, 0, 255}},    {"AMBER", {255, 112, 0}},
    {"WHITE", {255, 255, 255}}, {"OFF", {0, 0, 0}},
};
constexpr uint8_t kLedTestOutputCount = sizeof(kLedTestOutputs) / sizeof(kLedTestOutputs[0]);

}  // namespace

InputBuffer::InputBuffer() : line_{}, length_(0), discardingOversizedLine_(false) {}

InputEvent InputBuffer::append(char character) {
  if (character == '\r') return InputEvent::None;
  if (character == '\n') {
    const bool oversized = discardingOversizedLine_;
    line_[length_] = '\0';
    length_ = 0;
    discardingOversizedLine_ = false;
    return oversized ? InputEvent::Oversized : InputEvent::LineReady;
  }
  if (discardingOversizedLine_) return InputEvent::None;
  if (length_ >= kMaximumCommandLength) {
    length_ = 0;
    discardingOversizedLine_ = true;
    return InputEvent::None;
  }
  line_[length_++] = character;
  return InputEvent::None;
}

Command parseCommand(const char* line) {
  if (line == nullptr) return {CommandType::Invalid, tally::State::Off, 0};
  const char* value = skipSpaces(line);
  if (*value == '\0') return {CommandType::Empty, tally::State::Off, 0};
  if (readToken(value, "HELP") && hasNoMoreTokens(value)) return {CommandType::Help, tally::State::Off, 0};
  if (readToken(value, "STATUS") && hasNoMoreTokens(value)) return {CommandType::Status, tally::State::Off, 0};
  value = skipSpaces(line);
  if (readToken(value, "WIFI")) {
    value = skipSpaces(value);
    if (readToken(value, "STATUS") && hasNoMoreTokens(value))
      return {CommandType::WifiStatus, tally::State::Off, 0};
    return {CommandType::Invalid, tally::State::Off, 0};
  }
  if (readToken(value, "PROVISION") && hasNoMoreTokens(value)) return {CommandType::Provision, tally::State::Off, 0};
  if (readToken(value, "EXIT") && hasNoMoreTokens(value)) return {CommandType::ExitDiagnostics, tally::State::Off, 0};

  value = skipSpaces(line);
  if (readToken(value, "DIAGNOSTICS")) {
    value = skipSpaces(value);
    if (readToken(value, "ON") && hasNoMoreTokens(value)) return {CommandType::EnterDiagnostics, tally::State::Off, 0};
    if (readToken(value, "OFF") && hasNoMoreTokens(value)) return {CommandType::ExitDiagnostics, tally::State::Off, 0};
    return {CommandType::Invalid, tally::State::Off, 0};
  }
  value = skipSpaces(line);
  if (readToken(value, "LED")) {
    value = skipSpaces(value);
    if (readToken(value, "TEST") && hasNoMoreTokens(value)) return {CommandType::LedTest, tally::State::Off, 0};
    if (readToken(value, "RAINBOW") && hasNoMoreTokens(value)) return {CommandType::LedRainbow, tally::State::Off, 0};
    if (readToken(value, "STOP") && hasNoMoreTokens(value)) return {CommandType::LedStop, tally::State::Off, 0};
    if (readToken(value, "BRIGHTNESS")) {
      value = skipSpaces(value);
      if (*value == '\0') return {CommandType::LedBrightnessStatus, tally::State::Off, 0};
      uint8_t brightness = 0;
      return readBrightness(value, brightness) ? Command{CommandType::LedBrightness, tally::State::Off, brightness}
                                              : Command{CommandType::Invalid, tally::State::Off, 0};
    }
    return {CommandType::Invalid, tally::State::Off, 0};
  }
  value = skipSpaces(line);
  if (readToken(value, "TALLY")) {
    value = skipSpaces(value);
    const tally::State states[] = {tally::State::Program, tally::State::Preview,
                                   tally::State::Warning, tally::State::Identify, tally::State::Off};
    for (const tally::State state : states) {
      const char* afterState = value;
      if (readToken(afterState, tally::StateMachine::name(state)) && hasNoMoreTokens(afterState))
        return {CommandType::SetTally, state, 0};
    }
  }
  return {CommandType::Invalid, tally::State::Off, 0};
}

DiagnosticSession::DiagnosticSession()
    : active_(false), ledTestActive_(false), rainbowActive_(false), configuredBrightness_(0),
      diagnosticBrightness_(0), ledTestStep_(0), nextLedTestTransitionAt_(0),
      returnState_(tally::State::Off), preTestState_(tally::State::Off) {}

bool DiagnosticSession::enter(tally::State returnState, uint8_t configuredBrightness) {
  if (active_) return false;
  active_ = true;
  returnState_ = returnState;
  configuredBrightness_ = config::clampBrightness(configuredBrightness);
  diagnosticBrightness_ = configuredBrightness_;
  return true;
}

bool DiagnosticSession::exit(tally::State& restoredState) {
  if (!active_) return false;
  active_ = false;
  ledTestActive_ = false;
  rainbowActive_ = false;
  restoredState = returnState_;
  return true;
}

bool DiagnosticSession::setBrightness(uint8_t brightness) {
  if (!active_ || brightness != config::clampBrightness(brightness)) return false;
  diagnosticBrightness_ = brightness;
  return true;
}

bool DiagnosticSession::startLedTest(uint32_t now, tally::State returnState) {
  if (!active_ || ledTestActive_) return false;
  rainbowActive_ = false;
  ledTestActive_ = true;
  ledTestStep_ = 0;
  preTestState_ = returnState;
  nextLedTestTransitionAt_ = now + kLedTestStepMs;
  return true;
}

bool DiagnosticSession::cancelLedTest(tally::State& restoredState) {
  if (!ledTestActive_) return false;
  ledTestActive_ = false;
  restoredState = preTestState_;
  return true;
}

bool DiagnosticSession::startRainbow(tally::State returnState) {
  if (!active_) return false;
  ledTestActive_ = false;
  preTestState_ = returnState;
  rainbowActive_ = true;
  return true;
}

bool DiagnosticSession::stopRainbow(tally::State& restoredState) {
  if (!rainbowActive_) return false;
  rainbowActive_ = false;
  restoredState = preTestState_;
  return true;
}

bool DiagnosticSession::cancelEffects(tally::State& restoredState) {
  if (cancelLedTest(restoredState)) return true;
  return stopRainbow(restoredState);
}

bool DiagnosticSession::updateLedTest(uint32_t now, tally::State& restoredState) {
  if (!ledTestActive_ || static_cast<int32_t>(now - nextLedTestTransitionAt_) < 0) return false;
  if (ledTestStep_ + 1 < kLedTestOutputCount) {
    ++ledTestStep_;
    nextLedTestTransitionAt_ = now + kLedTestStepMs;
    return true;
  }
  ledTestActive_ = false;
  restoredState = preTestState_;
  return true;
}

LedTestOutput DiagnosticSession::ledTestOutput() const {
  return kLedTestOutputs[ledTestStep_];
}

}  // namespace serial
