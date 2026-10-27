#include <unity.h>

#include <cstring>

#include "config/DeviceConfig.h"
#include "config/ConfigRecord.h"
#include "config/ConfigRecovery.h"
#include "network/ConnectionState.h"
#include "platform/BoardConfiguration.h"
#include "security/ProvisioningAuth.h"
#include "serial/SerialDiagnostics.h"
#include "tally/TallyState.h"
#include "web/RequestValidation.h"

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

void test_serial_commands_are_case_insensitive_and_map_states() {
  const char* commands[] = {"tally program", "  tally  pReViEw ", "TALLY WARNING",
                            "TaLlY identify", "TALLY OFF"};
  const tally::State states[] = {tally::State::Program, tally::State::Preview,
                                 tally::State::Warning, tally::State::Identify, tally::State::Off};
  for (size_t index = 0; index < sizeof(commands) / sizeof(commands[0]); ++index) {
    const serial::Command command = serial::parseCommand(commands[index]);
    TEST_ASSERT_EQUAL(serial::CommandType::SetTally, command.type);
    TEST_ASSERT_EQUAL(states[index], command.state);
  }
  TEST_ASSERT_EQUAL(serial::CommandType::LedTest, serial::parseCommand("led test").type);
  TEST_ASSERT_EQUAL(serial::CommandType::Provision, serial::parseCommand("PrOvIsIoN").type);
  TEST_ASSERT_EQUAL(serial::CommandType::EnterDiagnostics, serial::parseCommand("diagnostics on").type);
  TEST_ASSERT_EQUAL(serial::CommandType::ExitDiagnostics, serial::parseCommand("EXIT").type);
}

void test_serial_commands_reject_invalid_and_empty_input() {
  TEST_ASSERT_EQUAL(serial::CommandType::Empty, serial::parseCommand(" \t").type);
  TEST_ASSERT_EQUAL(serial::CommandType::Invalid, serial::parseCommand("TALLY PURPLE").type);
  TEST_ASSERT_EQUAL(serial::CommandType::Invalid, serial::parseCommand("LED TEST NOW").type);
  TEST_ASSERT_EQUAL(serial::CommandType::Invalid, serial::parseCommand(nullptr).type);
}

void test_serial_input_buffer_accepts_lf_and_crlf() {
  serial::InputBuffer buffer;
  TEST_ASSERT_EQUAL(serial::InputEvent::None, buffer.append('H'));
  TEST_ASSERT_EQUAL(serial::InputEvent::None, buffer.append('E'));
  TEST_ASSERT_EQUAL(serial::InputEvent::None, buffer.append('\r'));
  TEST_ASSERT_EQUAL(serial::InputEvent::LineReady, buffer.append('\n'));
  TEST_ASSERT_EQUAL_STRING("HE", buffer.line());
}

void test_serial_input_buffer_rejects_oversized_commands() {
  serial::InputBuffer buffer;
  for (size_t index = 0; index <= serial::kMaximumCommandLength; ++index)
    TEST_ASSERT_EQUAL(serial::InputEvent::None, buffer.append('X'));
  TEST_ASSERT_EQUAL(serial::InputEvent::None, buffer.append('X'));
  TEST_ASSERT_EQUAL(serial::InputEvent::Oversized, buffer.append('\n'));
}

void test_diagnostic_session_requires_explicit_authorization_and_can_exit() {
  serial::DiagnosticSession session;
  TEST_ASSERT_FALSE(session.startLedTest(0, tally::State::Program));
  TEST_ASSERT_TRUE(session.enter(tally::State::Preview));
  TEST_ASSERT_TRUE(session.active());
  tally::State restored = tally::State::Off;
  TEST_ASSERT_TRUE(session.exit(restored));
  TEST_ASSERT_EQUAL(tally::State::Preview, restored);
  TEST_ASSERT_FALSE(session.active());
}

void test_led_test_cancellation_restores_previous_state() {
  serial::DiagnosticSession session;
  TEST_ASSERT_TRUE(session.enter(tally::State::Off));
  TEST_ASSERT_TRUE(session.startLedTest(100, tally::State::Warning));
  TEST_ASSERT_TRUE(session.ledTestActive());
  tally::State restored = tally::State::Off;
  TEST_ASSERT_TRUE(session.cancelLedTest(restored));
  TEST_ASSERT_EQUAL(tally::State::Warning, restored);
  TEST_ASSERT_FALSE(session.ledTestActive());
}

void test_led_test_sequences_without_blocking() {
  serial::DiagnosticSession session;
  TEST_ASSERT_TRUE(session.enter(tally::State::Off));
  TEST_ASSERT_TRUE(session.startLedTest(100, tally::State::Program));
  TEST_ASSERT_EQUAL_STRING("RED", session.ledTestOutput().name);
  tally::State restored = tally::State::Off;
  TEST_ASSERT_FALSE(session.updateLedTest(1099, restored));
  TEST_ASSERT_TRUE(session.updateLedTest(1100, restored));
  TEST_ASSERT_EQUAL_STRING("GREEN", session.ledTestOutput().name);
  TEST_ASSERT_TRUE(session.updateLedTest(2100, restored));
  TEST_ASSERT_EQUAL_STRING("BLUE", session.ledTestOutput().name);
  TEST_ASSERT_TRUE(session.updateLedTest(3100, restored));
  TEST_ASSERT_EQUAL_STRING("AMBER", session.ledTestOutput().name);
  TEST_ASSERT_TRUE(session.updateLedTest(4100, restored));
  TEST_ASSERT_EQUAL_STRING("WHITE", session.ledTestOutput().name);
  TEST_ASSERT_TRUE(session.updateLedTest(5100, restored));
  TEST_ASSERT_EQUAL_STRING("OFF", session.ledTestOutput().name);
  TEST_ASSERT_TRUE(session.updateLedTest(6100, restored));
  TEST_ASSERT_FALSE(session.ledTestActive());
  TEST_ASSERT_EQUAL(tally::State::Program, restored);
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
  TEST_ASSERT_EQUAL_UINT8(16, value.brightness);
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

void test_brightness_is_conservatively_clamped() {
  TEST_ASSERT_EQUAL_UINT8(32, config::clampBrightness(255));
  TEST_ASSERT_EQUAL_UINT8(16, config::clampBrightness(16));
  config::DeviceConfig value = config::defaults(2);
  value.brightness = 33;
  TEST_ASSERT_FALSE(config::isValid(value));
}

void test_board_led_pin_policy_keeps_platform_routes_distinct() {
  TEST_ASSERT_TRUE(platform::isLedPinValidForTarget(platform::BoardTarget::Esp8266Feather, 15));
  TEST_ASSERT_FALSE(platform::isLedPinValidForTarget(platform::BoardTarget::Esp8266Feather, 18));
  TEST_ASSERT_TRUE(platform::isLedPinValidForTarget(platform::BoardTarget::Esp32DevKit, 18));
  TEST_ASSERT_FALSE(platform::isLedPinValidForTarget(platform::BoardTarget::Esp32DevKit, 15));
}

void test_configuration_record_detects_corruption() {
  const config::ConfigRecord record = config::makeRecord(config::defaults(2));
  TEST_ASSERT_TRUE(config::isRecordValid(record));
  config::ConfigRecord corrupted = record;
  corrupted.configuration.brightness = 17;
  TEST_ASSERT_FALSE(config::isRecordValid(corrupted));
}

void test_backup_is_selected_when_the_primary_is_invalid() {
  TEST_ASSERT_EQUAL(config::RecoverySource::Primary, config::selectRecoverySource(true, true));
  TEST_ASSERT_EQUAL(config::RecoverySource::Backup, config::selectRecoverySource(false, true));
  TEST_ASSERT_EQUAL(config::RecoverySource::None, config::selectRecoverySource(false, false));
}

void test_provisioning_authorization_and_window_lifecycle() {
  constexpr char secret[] = "0123456789ABCDEF01234567";
  TEST_ASSERT_TRUE(security::secretMatches(secret, secret, 24));
  TEST_ASSERT_FALSE(security::secretMatches("0123456789ABCDEF01234568", secret, 24));
  TEST_ASSERT_FALSE(security::secretMatches("short", secret, 24));
  TEST_ASSERT_TRUE(security::provisioningWindowIsActive(100, 200));
  TEST_ASSERT_FALSE(security::provisioningWindowIsActive(200, 200));
  TEST_ASSERT_FALSE(security::provisioningWindowIsActive(100, 0));
}

void test_network_changes_are_distinguished_from_brightness_changes() {
  const config::DeviceConfig current = config::defaults(2);
  config::DeviceConfig brightnessChange = current;
  brightnessChange.brightness = 20;
  TEST_ASSERT_FALSE(config::networkSettingsChanged(current, brightnessChange));
  config::DeviceConfig credentialChange = current;
  config::copyBounded(credentialChange.wifiSsid, sizeof(credentialChange.wifiSsid), "studio");
  TEST_ASSERT_TRUE(config::networkSettingsChanged(current, credentialChange));
}

void test_request_validation_rejects_malformed_input() {
  unsigned long brightness = 0;
  unsigned long contentLength = 0;
  TEST_ASSERT_TRUE(web::validNetworkFields("Tally", "studio", "password"));
  TEST_ASSERT_FALSE(web::validNetworkFields("", "studio", "password"));
  TEST_ASSERT_TRUE(web::isDecimalInRange("256", 256, contentLength));
  TEST_ASSERT_EQUAL_UINT32(256, contentLength);
  TEST_ASSERT_FALSE(web::isDecimalInRange("257", 256, contentLength));
  TEST_ASSERT_FALSE(web::isDecimalInRange("2 56", 256, contentLength));
  TEST_ASSERT_FALSE(web::validTallyFields("PROGRAM", "33", brightness));
  TEST_ASSERT_FALSE(web::validTallyFields("PROGRAM", "3x", brightness));
  TEST_ASSERT_TRUE(web::validTallyFields("PROGRAM", "32", brightness));
  TEST_ASSERT_EQUAL_UINT32(32, brightness);
}

void test_connection_policy_uses_timeout_and_backoff_without_provisioning() {
  network::ConnectionStateMachine policy;
  TEST_ASSERT_EQUAL(network::ConnectionAction::Connect, policy.begin(true, 0));
  TEST_ASSERT_EQUAL(network::ConnectionState::Connecting, policy.state());
  TEST_ASSERT_EQUAL(network::ConnectionAction::None,
                    policy.update(false, network::ConnectionStateMachine::kConnectTimeoutMs));
  TEST_ASSERT_EQUAL(network::ConnectionState::Backoff, policy.state());
  TEST_ASSERT_EQUAL(network::ConnectionAction::Connect,
                    policy.update(false, network::ConnectionStateMachine::kConnectTimeoutMs +
                                             network::ConnectionStateMachine::kInitialBackoffMs));
  TEST_ASSERT_EQUAL(network::ConnectionAction::None, policy.update(true, 30000));
  TEST_ASSERT_EQUAL(network::ConnectionState::Connected, policy.state());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_initial_state_is_off);
  RUN_TEST(test_transition_changes_only_when_needed);
  RUN_TEST(test_state_parser_rejects_unknown_value);
  RUN_TEST(test_serial_commands_are_case_insensitive_and_map_states);
  RUN_TEST(test_serial_commands_reject_invalid_and_empty_input);
  RUN_TEST(test_serial_input_buffer_accepts_lf_and_crlf);
  RUN_TEST(test_serial_input_buffer_rejects_oversized_commands);
  RUN_TEST(test_diagnostic_session_requires_explicit_authorization_and_can_exit);
  RUN_TEST(test_led_test_cancellation_restores_previous_state);
  RUN_TEST(test_led_test_sequences_without_blocking);
  RUN_TEST(test_state_colors_match_documented_tally_states);
  RUN_TEST(test_default_configuration_is_valid_but_unprovisioned);
  RUN_TEST(test_configuration_rejects_invalid_schema_and_unterminated_name);
  RUN_TEST(test_bounded_copy_rejects_overflow);
  RUN_TEST(test_brightness_is_conservatively_clamped);
  RUN_TEST(test_board_led_pin_policy_keeps_platform_routes_distinct);
  RUN_TEST(test_configuration_record_detects_corruption);
  RUN_TEST(test_backup_is_selected_when_the_primary_is_invalid);
  RUN_TEST(test_provisioning_authorization_and_window_lifecycle);
  RUN_TEST(test_network_changes_are_distinguished_from_brightness_changes);
  RUN_TEST(test_request_validation_rejects_malformed_input);
  RUN_TEST(test_connection_policy_uses_timeout_and_backoff_without_provisioning);
  return UNITY_END();
}
