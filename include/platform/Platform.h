#pragma once

#include <stdint.h>

// Keep Arduino core differences at this boundary. Application, configuration,
// tally, and LED code stay independent of the selected Wi-Fi/web-server core.
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
using TallyWebServer = ESP8266WebServer;
#elif defined(ESP32)
#include <WebServer.h>
#include <WiFi.h>
using TallyWebServer = WebServer;
#else
#error "ESP-NMOS-TALLY requires an ESP8266 or ESP32 Arduino target."
#endif

namespace platform {

void prepareWiFi();
uint32_t secureRandom();
uint32_t freeHeap();
const char* name();

}  // namespace platform
