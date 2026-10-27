#include <Arduino.h>

#include <string.h>

#include "config/ConfigStore.h"
#include "led/NeoPixelRenderer.h"
#include "led/RainbowAnimation.h"
#include "network/NetworkController.h"
#include "platform/BoardConfiguration.h"
#include "serial/SerialDiagnostics.h"
#include "tally/TallyState.h"
#include "web/DeviceWebServer.h"

namespace {
constexpr uint8_t kLedPin = TALLY_LED_PIN;
constexpr uint8_t kLedCount = TALLY_LED_COUNT;
constexpr uint32_t kSelfTestStepMs = 300;
#if defined(ESP8266)
static_assert(platform::isLedPinValidForTarget(platform::BoardTarget::Esp8266Feather, kLedPin),
              "ESP8266 Feather firmware must use its documented GPIO 2 LED route.");
#elif defined(ESP32)
static_assert(platform::isLedPinValidForTarget(platform::BoardTarget::Esp32DevKit, kLedPin),
              "ESP32 DevKit/Wokwi firmware must use GPIO 18 for the LED matrix.");
#endif

class Application final : public web::Control {
 public:
  Application()
      : store_(kLedPin), configuration_(config::defaults(kLedPin)), renderer_(kLedPin, kLedCount),
        web_(*this), selfTestStep_(0), selfTestStartedAt_(0) {}

  void begin() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("ESP-NMOS-TALLY starting"));
    const bool filesystemReady = store_.begin();
    if (!filesystemReady) Serial.println(F("WARN: LittleFS unavailable; configuration will not persist"));
    if (filesystemReady && !store_.load(configuration_)) {
      Serial.println(F("INFO: No valid saved configuration. Type PROVISION on USB serial to begin setup."));
    }
    if (!renderer_.begin()) Serial.println(F("WARN: LED renderer unavailable"));
    network_.begin(configuration_);
    web_.begin();
    selfTestStartedAt_ = millis();
    renderIfNeeded();
  }

  void update() {
    processSerialCommands();
    network_.update();
    web_.handleClient();
    runSelfTest();
    renderIfNeeded();
    runRainbowAnimation();
  }

  const config::DeviceConfig& configuration() const override { return configuration_; }
  const network::Controller& network() const override { return network_; }
  tally::State state() const override { return state_.state(); }

  bool updateConfiguration(const config::DeviceConfig& value) override {
    if (!config::isValid(value)) return false;
    if (config::equals(configuration_, value)) return true;
    if (!store_.save(value)) return false;
    const bool reconnectRequired = config::networkSettingsChanged(configuration_, value);
    configuration_ = value;
    if (reconnectRequired) {
      network_.connect(configuration_);
      state_.set(tally::State::Off);
    }
    return true;
  }

  bool setManualState(tally::State state) override {
    selfTestStep_ = 0;
    state_.set(state);
    return true;
  }

 private:
  void processSerialCommands() {
    while (Serial.available() > 0) {
      const char received = static_cast<char>(Serial.read());
      const serial::InputEvent event = serialInput_.append(received);
      if (event == serial::InputEvent::Oversized) {
        Serial.println(F("ERROR: Command is too long."));
      } else if (event == serial::InputEvent::LineReady) {
        handleSerialCommand(serialInput_.line());
      }
    }
  }

  void handleSerialCommand(const char* line) {
    const serial::Command command = serial::parseCommand(line);
    switch (command.type) {
      case serial::CommandType::Empty: return;
      case serial::CommandType::Help: return printSerialHelp();
      case serial::CommandType::Status: return printSerialStatus();
      case serial::CommandType::Provision: return startProvisioning();
      case serial::CommandType::EnterDiagnostics: return enterDiagnostics();
      case serial::CommandType::ExitDiagnostics: return exitDiagnostics();
      case serial::CommandType::SetTally: return setDiagnosticTally(command.state);
      case serial::CommandType::LedTest: return startLedTest();
      case serial::CommandType::LedRainbow: return startRainbow();
      case serial::CommandType::LedStop: return stopLedEffect();
      case serial::CommandType::Invalid:
        Serial.println(F("ERROR: Unknown command. Type HELP."));
        return;
    }
  }

  void printSerialHelp() const {
    Serial.println(F("INFO: Commands: HELP, STATUS, PROVISION, DIAGNOSTICS ON, DIAGNOSTICS OFF/EXIT"));
    Serial.println(F("INFO: Diagnostics: TALLY PROGRAM|PREVIEW|WARNING|IDENTIFY|OFF, LED TEST|RAINBOW|STOP"));
  }

  void printSerialStatus() const {
    Serial.print(F("INFO: Tally state: "));
    Serial.println(tally::StateMachine::name(state_.state()));
    Serial.print(F("INFO: Diagnostics: "));
    Serial.println(diagnostics_.active() ? F("enabled") : F("disabled"));
    Serial.print(F("INFO: LED test: "));
    Serial.println(diagnostics_.ledTestActive() ? F("running") : F("idle"));
    Serial.print(F("INFO: LED rainbow: "));
    Serial.println(diagnostics_.rainbowActive() ? F("running") : F("idle"));
  }

  void startProvisioning() {
    if (!network_.startProvisioning()) {
      Serial.println(F("ERROR: Could not start provisioning access point."));
      return;
    }
    Serial.println(F("INFO: Provisioning AP active for 10 minutes."));
    Serial.print(F("INFO: SSID: "));
    Serial.println(network_.apSsid());
    Serial.print(F("INFO: Provisioning secret: "));
    Serial.println(network_.provisioningSecretForSerial());
  }

  void enterDiagnostics() {
    selfTestStep_ = 5;
    if (!diagnostics_.enter(state_.state())) {
      Serial.println(F("ERROR: Diagnostics are already enabled."));
      return;
    }
    Serial.println(F("INFO: Local serial diagnostics enabled. Use EXIT to return to application control."));
  }

  void exitDiagnostics() {
    tally::State restoredState = tally::State::Off;
    if (!diagnostics_.exit(restoredState)) {
      Serial.println(F("ERROR: Diagnostics are not enabled."));
      return;
    }
    rainbow_.stop();
    state_.set(restoredState);
    renderTallyOutput();
    Serial.println(F("INFO: Diagnostics disabled; returned to application control."));
  }

  void setDiagnosticTally(tally::State requestedState) {
    if (!diagnostics_.active()) {
      Serial.println(F("ERROR: Run DIAGNOSTICS ON before changing tally output."));
      return;
    }
    tally::State restoredState = tally::State::Off;
    if (diagnostics_.cancelEffects(restoredState)) {
      rainbow_.stop();
      Serial.println(F("INFO: LED diagnostic effect cancelled."));
    }
    state_.set(requestedState);
    renderTallyOutput();
    Serial.print(F("INFO: Tally state set to "));
    Serial.println(tally::StateMachine::name(requestedState));
    logRenderRequested(tally::StateMachine::name(requestedState));
  }

  void startLedTest() {
    if (!diagnostics_.active()) {
      Serial.println(F("ERROR: Run DIAGNOSTICS ON before LED TEST."));
      return;
    }
    if (!diagnostics_.startLedTest(millis(), state_.state())) {
      Serial.println(F("ERROR: LED test is already running."));
      return;
    }
    rainbow_.stop();
    renderLedTestOutput();
  }

  void startRainbow() {
    if (!diagnostics_.active()) {
      Serial.println(F("ERROR: Run DIAGNOSTICS ON before LED RAINBOW."));
      return;
    }
    if (!diagnostics_.startRainbow(state_.state())) {
      Serial.println(F("ERROR: Could not start LED RAINBOW."));
      return;
    }
    rainbow_.start(millis());
    renderRainbowFrame();
    Serial.println(F("INFO: LED rainbow started at 32/255. Use LED STOP or EXIT to cancel."));
  }

  void stopLedEffect() {
    if (!diagnostics_.active()) {
      Serial.println(F("ERROR: Run DIAGNOSTICS ON before LED STOP."));
      return;
    }
    tally::State restoredState = tally::State::Off;
    if (!diagnostics_.cancelEffects(restoredState)) {
      Serial.println(F("INFO: No diagnostic LED effect is running."));
      return;
    }
    rainbow_.stop();
    state_.set(restoredState);
    renderTallyOutput();
    Serial.println(F("INFO: LED diagnostic effect stopped; restored diagnostic tally state."));
  }

  void runLedTest() {
    tally::State restoredState = tally::State::Off;
    if (!diagnostics_.updateLedTest(millis(), restoredState)) return;
    if (diagnostics_.ledTestActive()) {
      renderLedTestOutput();
      return;
    }
    state_.set(restoredState);
    renderTallyOutput();
    Serial.println(F("INFO: LED test complete; restored diagnostic tally state."));
  }

  void runRainbowAnimation() {
    if (!diagnostics_.rainbowActive()) return;
    if (rainbow_.update(millis())) renderRainbowFrame();
  }

  void renderLedTestOutput() {
    const serial::LedTestOutput output = diagnostics_.ledTestOutput();
    renderer_.render(output.color, config::kMaximumBrightness);
    Serial.print(F("INFO: LED test output requested: "));
    Serial.print(output.name);
    Serial.println(F(" at 32/255; verify LEDs visually."));
  }

  void renderRainbowFrame() {
    tally::RgbColor colors[kLedCount];
    for (uint8_t pixel = 0; pixel < kLedCount; ++pixel)
      colors[pixel] = rainbow_.colorForPixel(pixel, kLedCount);
    renderer_.renderPixels(colors, kLedCount, config::kMaximumBrightness);
  }

  void logRenderRequested(const char* name) const {
    Serial.print(F("INFO: NeoPixel render requested: "));
    Serial.print(kLedCount);
    Serial.print(F(" pixels, "));
    Serial.print(name);
    Serial.println(F("; verify LEDs visually."));
  }

  void runSelfTest() {
    if (selfTestStep_ >= 5 || millis() - selfTestStartedAt_ < kSelfTestStepMs) return;
    const tally::State steps[] = {tally::State::Program, tally::State::Preview,
                                  tally::State::Warning, tally::State::Identify, tally::State::Off};
    state_.set(steps[selfTestStep_++]);
    selfTestStartedAt_ = millis();
  }

  void renderIfNeeded() {
    // A test drives the renderer directly, including white which is not a
    // tally state. Consume pending normal-state changes until it completes.
    if (diagnostics_.ledTestActive() || diagnostics_.rainbowActive()) {
      state_.consumeChange();
      return;
    }
    if (!state_.consumeChange()) return;
    renderTallyOutput();
  }

  void renderTallyOutput() {
    renderer_.render(state_.color(), configuration_.brightness);
  }

  config::Store store_;
  config::DeviceConfig configuration_;
  network::Controller network_;
  tally::StateMachine state_;
  led::NeoPixelRenderer renderer_;
  web::DeviceWebServer web_;
  serial::InputBuffer serialInput_;
  serial::DiagnosticSession diagnostics_;
  led::RainbowAnimation rainbow_;
  uint8_t selfTestStep_;
  uint32_t selfTestStartedAt_;
};

Application app;
}  // namespace

void setup() { app.begin(); }

void loop() { app.update(); }
