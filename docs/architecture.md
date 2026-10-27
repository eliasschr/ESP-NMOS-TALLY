# Architecture

The firmware keeps policy separate from hardware. `tally::StateMachine` is a small, Arduino-independent state/value component and is tested on the host. `led::NeoPixelRenderer` maps a state color to the 32-pixel FeatherWing. Network connection management and HTTP handling do not write LEDs directly; `Application` coordinates them in its non-blocking loop.

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

The current interfaces deliberately avoid pretending ESP8266 and ESP32 are interchangeable. Future ESP32 support should supply a platform-specific network, filesystem, and LED implementation while reusing the state/configuration logic after an actual ESP32 build and hardware validation.

The main loop only polls serial input, Wi-Fi, HTTP, the startup self-test, and renderer changes. There are no normal-operation delays or reconnect loops. The self-test advances every 300 ms and is therefore watchdog-friendly. Brightness-only and manual-state updates avoid station reconnects and skip persistence when unchanged.
