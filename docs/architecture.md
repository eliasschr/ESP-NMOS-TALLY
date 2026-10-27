# Architecture

The firmware keeps policy separate from hardware. `tally::StateMachine` is a small, Arduino-independent state/value component and is tested on the host. `led::NeoPixelRenderer` maps a state color to the 32-pixel FeatherWing. Network connection management and HTTP handling do not write LEDs directly; `Application` coordinates them in its non-blocking loop.

`serial::InputBuffer`, `parseCommand`, and `DiagnosticSession` likewise remain Arduino-independent. They bound and parse USB lines, gate diagnostics behind an explicit RAM-only session, and schedule the LED test without calling hardware APIs. `Application` is the only owner of serial I/O and calls the existing renderer for both normal tally states and the test sequence. This keeps parsing and authorization testable without creating a simulator-only tally or LED implementation.

```text
USB `PROVISION` -> Network controller -> temporary setup AP
                                           |
HTTP setup controls -> Application -> Tally State -> NeoPixel renderer
                         |
                         +-> Config store -> LittleFS
Wi-Fi controller --------+
```

`config::DeviceConfig` is a fixed-size, versioned payload inside a versioned `ConfigRecord`. The record has a CRC-32 checksum for accidental-corruption detection, not cryptographic authentication. The LittleFS store writes a temporary file, retains the previous valid primary as `/config.bak`, and loads that backup when the primary is absent or invalid. It does not promote a recovered backup automatically, avoiding an unnecessary flash write.

Provisioning is a serial-authorized, 10-minute session. `PROVISION` causes the ESP8266 SDK RNG to generate a 96-bit secret; the secret is printed only to USB serial and is used for WPA2 AP access and the `X-Provisioning-Secret` header. The browser keeps it only in page memory. State-changing requests also require a same-origin request and are rate-limited after repeated failed authorization. The station-network dashboard is read-only; state-changing routes are limited to the setup session, so there is no remote management mode.

`network::ConnectionStateMachine` is Arduino-independent and selects non-blocking station reconnects: a 20-second attempt, then a bounded burst of three attempts with exponential backoff from 2 seconds. A failed burst cools down for 60 seconds before another bounded burst. It never opens the AP for a station failure. It does not represent a future MQTT/NMOS event-source health check; when such a source exists it must independently force a WARNING state after its own stale-data timeout.

When no station configuration exists, the ESP32 initializes an unconnected STA interface while the HTTP listener is created. Arduino-ESP32 initializes its lwIP TCP/IP task with that interface; selecting `WIFI_OFF` from the initial null mode leaves the task uninitialized and makes listener initialization assert. This is not a connection attempt and does not create an AP. Provisioning remains explicitly serial-authorized.

`platform::Platform` is the intentionally small platform boundary. It selects the ESP8266 or ESP32 Wi-Fi/web-server headers, disables implicit Wi-Fi credential persistence, exposes free-heap reporting, and chooses the appropriate secure provisioning entropy source. The ESP8266 uses SDK `os_random()`; ESP32 uses its hardware `esp_random()`. LittleFS and Adafruit NeoPixel retain the same interfaces on both Arduino cores, so the configuration store, dashboard assets, tally state machine, rendering policy, and application loop remain shared. The board environment supplies the LED pin: GPIO 2 for the Feather ESP8266 and GPIO 18 for the generic ESP32. GPIO 2 remains a boot-strapping pin and shares the Feather's blue LED; the hardware constraints are documented in the hardware guide.

The main loop only polls serial input, Wi-Fi, HTTP, the startup self-test, and renderer changes. There are no normal-operation delays or reconnect loops. The self-test advances every 300 ms and is therefore watchdog-friendly. Brightness-only and manual-state updates avoid station reconnects and skip persistence when unchanged.
