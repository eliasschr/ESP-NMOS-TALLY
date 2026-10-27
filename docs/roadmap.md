# Roadmap

## Current foundation

- ESP8266 PlatformIO configuration
- LittleFS-backed versioned configuration
- USB-serial-authorized, time-limited local Wi-Fi provisioning
- Read-only device status dashboard after station connection
- Manual local tally test in provisioning mode
- Non-blocking self-test and NeoPixel rendering
- Native logic tests and CI build/test workflow
- Shared ESP8266/ESP32 Arduino firmware build targets
- Wokwi ESP32 circuit, full-image generation tooling, and first successful browser runtime check
- First limited physical ESP8266/NeoPixel FeatherWing check: USB flashing, serial diagnostics, and visible GPIO 2 LED output

## Next development phase

1. Add a physically guarded factory-reset flow.
2. Replace or extend HTTP handling if strict pre-parse request-body limits are required.
3. Add MQTT transport with bounded reconnect/resubscribe behavior.
4. Add strict NMOS IS-07 event validation and deployment-specific source mapping.
5. Complete physical ESP8266 tests: browser WLAN onboarding, configuration persistence, HTTP/authentication behavior, Wi-Fi-loss recovery, electrical current measurement, and long-term reliability.
6. Run an ESP32 DevKit and physical 4×8 matrix verification, including power, signal-integrity, and boot-strapping checks.
