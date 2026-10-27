# ESP-NMOS-TALLY

ESP-NMOS-TALLY is a Wi-Fi-connected tally-light firmware foundation for ESP8266 and ESP32. The physical target is an Adafruit Feather HUZZAH ESP8266 with an Adafruit NeoPixel FeatherWing 4×8 RGB; the ESP32 target enables the same firmware in PlatformIO and Wokwi.

It is not yet an NMOS device. MQTT and AMWA IS-07 integration are planned separately; no NMOS interoperability or conformance claim is made here.

## Status

| Capability | Implemented | Build-verified | Unit-tested | Hardware-tested |
| --- | --- | --- | --- | --- |
| ESP8266 firmware, serial-authorized provisioning, status dashboard | Yes | Yes | Yes | No |
| ESP32 `esp32dev` firmware | Yes | Yes | Shared logic | No |
| Wokwi ESP32 circuit and merged-image build tooling | Yes | Yes | N/A | No |
| Wokwi browser execution | Configuration supplied | Not run | N/A | N/A |
| Manual tally controls and NeoPixel renderer | Yes | Yes | Yes | No |
| Versioned configuration validation | Yes | Yes | Yes | No |
| MQTT / NMOS IS-07 | No, planned | N/A | N/A | No |
| ESP32 physical hardware support | Code target only | No | N/A | No |

## Hardware

Supported initial target:

- Adafruit Feather HUZZAH ESP8266
- Adafruit NeoPixel FeatherWing 4×8 RGB (32 pixels)
- USB power and Wi-Fi

See [hardware notes](docs/hardware.md) before powering the display at high brightness.

## Build and flash

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), then run:

```sh
pio run -e feather_huzzah_esp8266
pio run -e feather_huzzah_esp8266 -t upload
pio run -e feather_huzzah_esp8266 -t uploadfs
```

`uploadfs` installs the dashboard files from `data/www`; it is required for the browser interface. The firmware uses `littlefs` as configured in `platformio.ini`.

`platformio.ini` pins the ESP8266 platform, the native test platform, and Adafruit NeoPixel dependency. Use the exact checked-in file for reproducible builds.

For the ESP32 DevKit/Wokwi target:

```sh
pio run -e esp32dev
./scripts/build-wokwi.sh
```

The latter builds the application and its LittleFS image, then emits an ignored complete flash image at `dist/esp-nmos-tally-wokwi.bin`. See [Wokwi simulation](docs/wokwi-simulation.md) for import/upload steps and simulation limitations.

## First start

Open the USB serial console at 115200 baud and enter `PROVISION`. The device prints a new 24-character secret and starts `ESP-NMOS-TALLY-SETUP` for ten minutes. Use that secret to join the AP and enter it again in the setup page. On a successful network-credential change the device stops its setup AP; the dashboard remains read-only on the joined network.

The AP never starts automatically after a normal station disconnect. USB serial access can therefore recover a device without knowing previous Wi-Fi credentials. See [getting started](docs/getting-started.md) and [security notes](docs/security.md).

## Tally states

| State | Display color |
| --- | --- |
| OFF | Off |
| PROGRAM | Red |
| PREVIEW | Green |
| WARNING | Amber |
| IDENTIFY | Blue |

These application display states are not an AMWA IS-07 color mapping.

## Tests

```sh
pio test -e native
```

Native tests cover state/configuration validation, corruption detection, brightness limits, request validation, and reconnection policy. Firmware compilation and native tests do not validate ESP8266 Wi-Fi, HTTP parser behavior, LED, or power behavior.

Successful CI runs retain the ESP8266 `firmware.bin` as a workflow artifact for inspection or controlled test flashing; they do not publish a release or deploy firmware.

## Documentation

- [Architecture](docs/architecture.md)
- [Getting started](docs/getting-started.md)
- [Hardware](docs/hardware.md)
- [NMOS integration](docs/nmos-integration.md)
- [Security](docs/security.md)
- [Roadmap](docs/roadmap.md)
- [Wokwi simulation](docs/wokwi-simulation.md)

## License

No license has been selected or included yet. Until a license is added by the repository owner, this repository must not be represented as open-source licensed.
