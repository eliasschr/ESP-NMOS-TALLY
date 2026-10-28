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
- Physical ESP8266/NeoPixel FeatherWing Phase 2 check: USB/CP2104 serial diagnostics, GPIO 2 matrix tally output, browser provisioning, station DHCP, status dashboard, saved-credential reset recovery, and hotspot-loss reconnect

## Next development phase

1. Add a physically guarded factory-reset flow.
2. Replace or extend HTTP handling if strict pre-parse request-body limits are required.
3. Add MQTT transport with bounded reconnect/resubscribe behavior.
4. Add strict NMOS IS-07 event validation and deployment-specific source mapping.
5. Extend physical ESP8266 testing to router interoperability, authentication/error-path coverage, electrical current measurement, and long-term reliability.
6. Run an ESP32 DevKit and physical 4×8 matrix verification, including power, signal-integrity, and boot-strapping checks.
