#include <unity.h>

#include <cstring>

#include "config/DeviceConfig.h"
#include "tally/TallyState.h"

void test_initial_state_is_off() {
  tally::StateMachine state;
  TEST_ASSERT_EQUAL(tally::State::Off, state.state());
  TEST_ASSERT_TRUE(state.consumeChange());
  TEST_ASSERT_FALSE(state.consumeChange());
}

void test_transition_changes_only_when_needed() {
  tally::StateMachine state;
  TEST_ASSERT_TRUE(state.set(tally::State::Program));
  TEST_ASSERT_EQUAL_STRING("PROGRAM", tally::StateMachine::name(state.state()));
  TEST_ASSERT_FALSE(state.set(tally::State::Program));
  TEST_ASSERT_TRUE(state.consumeChange());
  TEST_ASSERT_FALSE(state.consumeChange());
}

void test_state_parser_rejects_unknown_value() {
  tally::State parsed = tally::State::Off;
  TEST_ASSERT_FALSE(tally::StateMachine::parse("program", parsed));
  TEST_ASSERT_FALSE(tally::StateMachine::parse(nullptr, parsed));
  TEST_ASSERT_TRUE(tally::StateMachine::parse("WARNING", parsed));
  TEST_ASSERT_EQUAL(tally::State::Warning, parsed);
}

void test_state_colors_match_documented_tally_states() {
  tally::StateMachine state;
  state.set(tally::State::Program);
  const tally::RgbColor program = state.color();
  TEST_ASSERT_EQUAL_UINT8(255, program.red);
  TEST_ASSERT_EQUAL_UINT8(0, program.green);
  state.set(tally::State::Warning);
  const tally::RgbColor warning = state.color();
  TEST_ASSERT_EQUAL_UINT8(255, warning.red);
  TEST_ASSERT_EQUAL_UINT8(112, warning.green);
  TEST_ASSERT_EQUAL_UINT8(0, warning.blue);
}

void test_default_configuration_is_valid_but_unprovisioned() {
  const config::DeviceConfig value = config::defaults(2);
  TEST_ASSERT_TRUE(config::isValid(value));
  TEST_ASSERT_FALSE(config::hasNetwork(value));
  TEST_ASSERT_EQUAL_UINT8(64, value.brightness);
}

void test_configuration_rejects_invalid_schema_and_unterminated_name() {
  config::DeviceConfig value = config::defaults(2);
  value.schemaVersion = 99;
  TEST_ASSERT_FALSE(config::isValid(value));
  value = config::defaults(2);
  memset(value.deviceName, 'x', sizeof(value.deviceName));
  TEST_ASSERT_FALSE(config::isValid(value));
}

void test_bounded_copy_rejects_overflow() {
  char buffer[4] = {};
  TEST_ASSERT_TRUE(config::copyBounded(buffer, sizeof(buffer), "abc"));
  TEST_ASSERT_EQUAL_STRING("abc", buffer);
  TEST_ASSERT_FALSE(config::copyBounded(buffer, sizeof(buffer), "abcd"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_initial_state_is_off);
  RUN_TEST(test_transition_changes_only_when_needed);
  RUN_TEST(test_state_parser_rejects_unknown_value);
  RUN_TEST(test_state_colors_match_documented_tally_states);
  RUN_TEST(test_default_configuration_is_valid_but_unprovisioned);
  RUN_TEST(test_configuration_rejects_invalid_schema_and_unterminated_name);
  RUN_TEST(test_bounded_copy_rejects_overflow);
  return UNITY_END();
}
