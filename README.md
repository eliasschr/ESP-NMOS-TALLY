# ESP-NMOS-TALLY

ESP-NMOS-TALLY is a Wi-Fi-connected tally-light firmware foundation for ESP8266 and ESP32. The physical target is an Adafruit Feather HUZZAH ESP8266 with an Adafruit NeoPixel FeatherWing 4×8 RGB; the ESP32 target enables the same firmware in PlatformIO and Wokwi.

It is not yet an NMOS device. MQTT and AMWA IS-07 integration are planned separately; no NMOS interoperability or conformance claim is made here.

## Status

| Capability | Implemented | Build-verified | Unit-tested | Hardware-tested |
| --- | --- | --- | --- | --- |
| ESP8266 firmware, serial-authorized provisioning, status dashboard | Yes | Yes | Yes | Partial: USB, flashing, serial diagnostics, and LED output |
| ESP32 `esp32dev` firmware | Yes | Yes | Shared logic | No |
| Wokwi ESP32 circuit and merged-image build tooling | Yes | Yes | N/A | No |
| Wokwi browser execution (ESP32) | Yes | Manual simulation | N/A | No |
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
The first physical ESP8266/FeatherWing result is recorded in [hardware test results](docs/hardware-test-results.md); it is not a full network, power, or long-term reliability qualification.

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

The first browser runtime check was run against firmware commit `0a9058e59b7d102b6a68dee2c8c6679eb04e5ac7`. It confirmed an ESP32 boot, LittleFS initialization, serial provisioning activation, and manual serial tally colors on the simulated 4×8 matrix. This is a Wokwi-only result; it does not verify physical ESP32 hardware, Wi-Fi, HTTP, MQTT, or NMOS behavior. A separate, limited physical ESP8266 LED and serial check is documented in the hardware test results.

## First start

Open the USB serial console at 115200 baud and enter `PROVISION`. The device prints a new 24-character secret and starts `ESP-NMOS-TALLY-SETUP` for ten minutes. Use that secret to join the AP and enter it again in the setup page. On a successful network-credential change the device stops its setup AP; the dashboard remains read-only on the joined network.

The AP never starts automatically after a normal station disconnect. USB serial access can therefore recover a device without knowing previous Wi-Fi credentials. See [getting started](docs/getting-started.md) and [security notes](docs/security.md).

## Local serial diagnostics

The same 115200-baud USB serial interface can exercise the real tally state machine and NeoPixel renderer without Wi-Fi, HTTP, or MQTT. Enter `DIAGNOSTICS ON` first; this intentionally limits LED control to a locally authorized serial session. `EXIT` (or `DIAGNOSTICS OFF`) restores the tally state from before the session and returns control to the normal application path. The session is never saved to flash.

| Command | Purpose |
| --- | --- |
| `HELP` | List commands. |
| `STATUS` | Show only the current tally and diagnostic-test state; it never prints credentials or secrets. |
| `PROVISION` | Preserve the existing serial-authorized Wi-Fi provisioning flow. |
| `DIAGNOSTICS ON` | Authorize one temporary local diagnostic session. |
| `TALLY PROGRAM`, `TALLY PREVIEW`, `TALLY WARNING`, `TALLY IDENTIFY`, `TALLY OFF` | Set the temporary diagnostic tally output. |
| `LED TEST` | Run red, green, blue, amber, brightness-limited white, and off for about one second each. It replaces a rainbow effect. |
| `LED RAINBOW` | Start a continuously moving full-matrix RGB hue gradient for LED diagnostics. |
| `LED STOP` | Stop the active diagnostic LED effect and restore the diagnostic tally state. |
| `EXIT` / `DIAGNOSTICS OFF` | Cancel an active diagnostic LED effect and leave diagnostics. |

Commands are case-insensitive and line-delimited. The firmware confirms render requests, not physical LED success; visually verify the matrix. `LED TEST` and `LED RAINBOW` are non-blocking and accept subsequent commands. The rainbow assigns a distinct HSV hue to every logical pixel and shifts the complete gradient at about 25 FPS. It always uses the existing conservative 32/255 physical brightness cap and never changes the saved brightness setting.

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

Native tests cover state/configuration validation, corruption detection, brightness limits, request validation, reconnection policy, bounded serial parsing, diagnostic authorization, and non-blocking LED-test sequencing. Firmware compilation and native tests do not validate ESP8266 Wi-Fi, HTTP parser behavior, LED, or power behavior.

Successful CI runs retain the ESP8266 `firmware.bin` as a workflow artifact for inspection or controlled test flashing; they do not publish a release or deploy firmware.

## Documentation

- [Architecture](docs/architecture.md)
- [Getting started](docs/getting-started.md)
- [Hardware](docs/hardware.md)
- [Hardware test results](docs/hardware-test-results.md)
- [NMOS integration](docs/nmos-integration.md)
- [Security](docs/security.md)
- [Roadmap](docs/roadmap.md)
- [Wokwi simulation](docs/wokwi-simulation.md)

## License

No license has been selected or included yet. Until a license is added by the repository owner, this repository must not be represented as open-source licensed.
