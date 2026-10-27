# Roadmap

## Current foundation

- ESP8266 PlatformIO configuration
- LittleFS-backed versioned configuration
- Recoverable local Wi-Fi provisioning
- Read-only device status dashboard after station connection
- Manual local tally test in provisioning mode
- Non-blocking self-test and NeoPixel rendering
- Native logic tests and CI build/test workflow

## Next development phase

1. Add a physically guarded factory-reset flow.
2. Improve provisioning credential handling and conduct a focused security review.
3. Add MQTT transport with bounded reconnect/resubscribe behavior.
4. Add strict NMOS IS-07 event validation and deployment-specific source mapping.
5. Run real Wi-Fi, LED, power, and interoperability tests.
6. Evaluate an ESP32 platform layer only after a separately verified build and hardware target are available.
