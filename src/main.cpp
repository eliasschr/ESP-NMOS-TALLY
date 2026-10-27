#include <Arduino.h>

#include "config/ConfigStore.h"
#include "led/NeoPixelRenderer.h"
#include "network/NetworkController.h"
#include "tally/TallyState.h"
#include "web/DeviceWebServer.h"

namespace {
constexpr uint8_t kLedPin = TALLY_LED_PIN;
constexpr uint8_t kLedCount = TALLY_LED_COUNT;
constexpr uint32_t kSelfTestStepMs = 300;

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
      Serial.println(F("INFO: No valid saved configuration; entering provisioning mode"));
    }
    if (!renderer_.begin()) Serial.println(F("WARN: LED renderer unavailable"));
    network_.begin(configuration_, ESP.getChipId());
    web_.begin();
    selfTestStartedAt_ = millis();
    renderIfNeeded();
  }

  void update() {
    network_.update();
    web_.handleClient();
    runSelfTest();
    renderIfNeeded();
  }

  const config::DeviceConfig& configuration() const override { return configuration_; }
  const network::Controller& network() const override { return network_; }
  tally::State state() const override { return state_.state(); }

  bool updateConfiguration(const config::DeviceConfig& value) override {
    if (!config::isValid(value) || !store_.save(value)) return false;
    configuration_ = value;
    network_.connect(configuration_);
    state_.set(tally::State::Off);
    return true;
  }

  bool setManualState(tally::State state) override {
    selfTestStep_ = 0;
    state_.set(state);
    return true;
  }

 private:
  void runSelfTest() {
    if (selfTestStep_ >= 5 || millis() - selfTestStartedAt_ < kSelfTestStepMs) return;
    const tally::State steps[] = {tally::State::Program, tally::State::Preview,
                                  tally::State::Warning, tally::State::Identify, tally::State::Off};
    state_.set(steps[selfTestStep_++]);
    selfTestStartedAt_ = millis();
  }

  void renderIfNeeded() {
    if (!state_.consumeChange()) return;
    renderer_.render(state_.color(), configuration_.brightness);
  }

  config::Store store_;
  config::DeviceConfig configuration_;
  network::Controller network_;
  tally::StateMachine state_;
  led::NeoPixelRenderer renderer_;
  web::DeviceWebServer web_;
  uint8_t selfTestStep_;
  uint32_t selfTestStartedAt_;
};

Application app;
}  // namespace

void setup() { app.begin(); }

void loop() { app.update(); }
