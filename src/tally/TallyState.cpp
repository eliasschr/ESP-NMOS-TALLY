#include "tally/TallyState.h"

#include <string.h>

namespace tally {

bool StateMachine::set(State state) {
  if (state_ == state) return false;
  state_ = state;
  changed_ = true;
  return true;
}

bool StateMachine::consumeChange() {
  const bool changed = changed_;
  changed_ = false;
  return changed;
}

RgbColor StateMachine::color() const {
  switch (state_) {
    case State::Program: return {255, 0, 0};
    case State::Preview: return {0, 255, 0};
    case State::Warning: return {255, 112, 0};
    case State::Identify: return {0, 0, 255};
    case State::Off: return {0, 0, 0};
  }
  return {0, 0, 0};
}

const char* StateMachine::name(State state) {
  switch (state) {
    case State::Off: return "OFF";
    case State::Program: return "PROGRAM";
    case State::Preview: return "PREVIEW";
    case State::Warning: return "WARNING";
    case State::Identify: return "IDENTIFY";
  }
  return "OFF";
}

bool StateMachine::parse(const char* value, State& state) {
  if (value == nullptr) return false;
  const State states[] = {State::Off, State::Program, State::Preview,
                          State::Warning, State::Identify};
  for (State candidate : states) {
    if (strcmp(value, name(candidate)) == 0) {
      state = candidate;
      return true;
    }
  }
  return false;
}

}  // namespace tally
