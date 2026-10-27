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
  if (line == nullptr) return {CommandType::Invalid, tally::State::Off};
  const char* value = skipSpaces(line);
  if (*value == '\0') return {CommandType::Empty, tally::State::Off};
  if (readToken(value, "HELP") && hasNoMoreTokens(value)) return {CommandType::Help, tally::State::Off};
  if (readToken(value, "STATUS") && hasNoMoreTokens(value)) return {CommandType::Status, tally::State::Off};
  if (readToken(value, "PROVISION") && hasNoMoreTokens(value)) return {CommandType::Provision, tally::State::Off};
  if (readToken(value, "EXIT") && hasNoMoreTokens(value)) return {CommandType::ExitDiagnostics, tally::State::Off};

  value = skipSpaces(line);
  if (readToken(value, "DIAGNOSTICS")) {
    value = skipSpaces(value);
    if (readToken(value, "ON") && hasNoMoreTokens(value)) return {CommandType::EnterDiagnostics, tally::State::Off};
    if (readToken(value, "OFF") && hasNoMoreTokens(value)) return {CommandType::ExitDiagnostics, tally::State::Off};
    return {CommandType::Invalid, tally::State::Off};
  }
  value = skipSpaces(line);
  if (readToken(value, "LED")) {
    value = skipSpaces(value);
    return readToken(value, "TEST") && hasNoMoreTokens(value)
               ? Command{CommandType::LedTest, tally::State::Off}
               : Command{CommandType::Invalid, tally::State::Off};
  }
  value = skipSpaces(line);
  if (readToken(value, "TALLY")) {
    value = skipSpaces(value);
    const tally::State states[] = {tally::State::Program, tally::State::Preview,
                                   tally::State::Warning, tally::State::Identify, tally::State::Off};
    for (const tally::State state : states) {
      const char* afterState = value;
      if (readToken(afterState, tally::StateMachine::name(state)) && hasNoMoreTokens(afterState))
        return {CommandType::SetTally, state};
    }
  }
  return {CommandType::Invalid, tally::State::Off};
}

DiagnosticSession::DiagnosticSession()
    : active_(false), ledTestActive_(false), ledTestStep_(0), nextLedTestTransitionAt_(0),
      returnState_(tally::State::Off), preTestState_(tally::State::Off) {}

bool DiagnosticSession::enter(tally::State returnState) {
  if (active_) return false;
  active_ = true;
  returnState_ = returnState;
  return true;
}

bool DiagnosticSession::exit(tally::State& restoredState) {
  if (!active_) return false;
  active_ = false;
  ledTestActive_ = false;
  restoredState = returnState_;
  return true;
}

bool DiagnosticSession::startLedTest(uint32_t now, tally::State returnState) {
  if (!active_ || ledTestActive_) return false;
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
