#include <Arduino.h>

#include <string.h>

#include "config/ConfigStore.h"
#include "led/NeoPixelRenderer.h"
#include "network/NetworkController.h"
#include "platform/BoardConfiguration.h"
#include "tally/TallyState.h"
#include "web/DeviceWebServer.h"

namespace {
constexpr uint8_t kLedPin = TALLY_LED_PIN;
constexpr uint8_t kLedCount = TALLY_LED_COUNT;
constexpr uint32_t kSelfTestStepMs = 300;
#if defined(ESP8266)
static_assert(platform::isLedPinValidForTarget(platform::BoardTarget::Esp8266Feather, kLedPin),
              "ESP8266 Feather firmware must use its documented GPIO 15 LED route.");
#elif defined(ESP32)
static_assert(platform::isLedPinValidForTarget(platform::BoardTarget::Esp32DevKit, kLedPin),
              "ESP32 DevKit/Wokwi firmware must use GPIO 18 for the LED matrix.");
#endif

class Application final : public web::Control {
 public:
  Application()
      : store_(kLedPin), configuration_(config::defaults(kLedPin)), renderer_(kLedPin, kLedCount),
        web_(*this), selfTestStep_(0), selfTestStartedAt_(0), serialCommandLength_(0) {}

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
      if (received == '\r') continue;
      if (received == '\n') {
        serialCommand_[serialCommandLength_] = '\0';
        handleSerialCommand();
        serialCommandLength_ = 0;
        continue;
      }
      if (serialCommandLength_ + 1 < sizeof(serialCommand_)) serialCommand_[serialCommandLength_++] = received;
      else serialCommandLength_ = 0;
    }
  }

  void handleSerialCommand() {
    if (strcmp(serialCommand_, "PROVISION") != 0) {
      if (serialCommandLength_ > 0) Serial.println(F("ERROR: Unknown command. Use PROVISION."));
      return;
    }
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
  char serialCommand_[16];
  size_t serialCommandLength_;
};

Application app;
}  // namespace

void setup() { app.begin(); }

void loop() { app.update(); }
