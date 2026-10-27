#pragma once

#include <stdint.h>

namespace tally {

enum class State : uint8_t { Off, Program, Preview, Warning, Identify };

struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

class StateMachine {
 public:
  StateMachine() : state_(State::Off), changed_(true) {}

  State state() const { return state_; }
  bool set(State state);
  bool consumeChange();
  RgbColor color() const;
  static const char* name(State state);
  static bool parse(const char* value, State& state);

 private:
  State state_;
  bool changed_;
};

}  // namespace tally
