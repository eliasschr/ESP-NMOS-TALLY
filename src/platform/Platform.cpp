#include "platform/Platform.h"

#if defined(ESP8266)
#include <user_interface.h>
#elif defined(ESP32)
#include <esp_system.h>
#endif

namespace platform {

void prepareWiFi() {
  // Do not allow the Arduino core to write credentials as a side effect of a
  // connection attempt. Configuration persistence belongs exclusively to Store.
  WiFi.persistent(false);
}

uint32_t secureRandom() {
#if defined(ESP8266)
  // ESP8266 SDK RNG is RF-noise backed. Arduino random() is not appropriate
  // for provisioning credentials.
  return os_random();
#elif defined(ESP32)
  // ESP32's hardware RNG is available before Wi-Fi association and is suitable
  // for generating the provisioning secret.
  return esp_random();
#endif
}

uint32_t freeHeap() { return ESP.getFreeHeap(); }

const char* name() {
#if defined(ESP8266)
  return "ESP8266";
#elif defined(ESP32)
  return "ESP32";
#endif
}

}  // namespace platform
