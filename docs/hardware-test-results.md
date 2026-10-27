# ESP8266 hardware test results

## Scope

This document records the first successful physical test of the ESP8266 tally assembly. It distinguishes observed behavior from unverified behavior; it is not a production qualification, electrical safety assessment, or NMOS conformance claim.

## Test record

| Item | Result |
| --- | --- |
| Test date | 2026-10-27 |
| Test platform | ESP-NMOS-TALLY on Adafruit Feather HUZZAH ESP8266 |
| Display hardware | Adafruit NeoPixel FeatherWing 4×8 RGB, 32 RGB LEDs |
| Firmware source tested | `2fbff0968ed17f69638fe09b686bf06795636cb5` |
| Development host | macOS with PlatformIO Core 6.2.0 |
| Transport | USB power and CP2104 USB-to-UART serial interface at 115200 baud |

No device MAC address, USB serial number, provisioning secret, or Wi-Fi credential is recorded here.

## Observed results

### USB, build, and upload

- After installation of the Silicon Labs CP210x VCP driver, macOS recognized the Feather's CP2104 USB-to-UART interface.
- PlatformIO compiled the firmware successfully and uploaded it through USB.
- The upload hash verification succeeded.
- The LittleFS image uploaded and verified successfully.
- PlatformIO Serial Monitor operated at 115200 baud.

### Firmware startup and diagnostics

- The ESP-NMOS-TALLY startup message appeared on serial.
- Missing initial configuration was handled.
- Serial diagnostics, including diagnostic-mode activation, operated successfully.

### User-confirmed diagnostic behavior

The following physical observations were confirmed by the user on the hardware
listed above. They are not automated test results:

- Firmware startup and USB/CP2104 serial diagnostics succeeded.
- The 32-pixel NeoPixel matrix displayed tally commands and the animated rainbow.
- `LED BRIGHTNESS 8` and `LED BRIGHTNESS 32` adjusted diagnostic output live.
- `LED STOP` and `EXIT` completed successfully.

The test did not verify WLAN onboarding, MQTT, NMOS IS-07, current consumption,
or long-term reliability.

### LED output and GPIO discovery

The tested FeatherWing had previously been modified: its NeoPixel data jumper was connected to GPIO 2, rather than GPIO 15. Selecting GPIO 2 in the ESP8266 firmware made the physical 4×8 NeoPixel matrix respond to firmware-driven LED output. Serial tally diagnostics also operated, and the user confirmed successful visible LED operation.

GPIO 2 is therefore the confirmed working data pin for this particular hardware assembly. It must not be generalized to all FeatherWings: the other FeatherWing in inventory has not been electrically verified.

## Pin and output policy

- ESP8266 Feather NeoPixel data pin: GPIO 2 for this verified assembly.
- ESP32 DevKit/Wokwi NeoPixel data pin: GPIO 18.
- Pixel count: 32.
- Maximum firmware brightness: 32/255.

The GPIO selection is centralized in `include/platform/BoardConfiguration.h` and passed by each PlatformIO environment as `TALLY_LED_PIN`. The 32/255 cap remains unchanged. It constrains rendered intensity but is not a measured electrical-current guarantee.

GPIO 2 is an ESP8266 boot-strapping pin and shares the Feather HUZZAH's onboard blue LED. It must remain high during reset; external loads or pull-downs that compromise that level can prevent boot. NeoPixel data activity may also make the onboard LED show activity. These constraints were documented, but this test did not measure timing margins or establish long-term boot reliability.

## macOS USB troubleshooting

If the CP2104 serial interface does not appear after connecting the Feather:

1. Install the current Silicon Labs CP210x VCP driver for macOS.
2. Reconnect the board and allow macOS to enumerate the device.
3. Select the resulting serial device in PlatformIO and use 115200 baud.
4. If enumeration still fails, check the USB cable supports data and try a known-good USB port before changing firmware or hardware wiring.

Do not place USB serial numbers or provisioning output in issue reports or commits.

## Not verified by this test

- Full WLAN onboarding from a browser.
- Configuration persistence across restarts.
- HTTP API behavior on physical hardware.
- Authentication expiry on physical hardware.
- Reconnect after Wi-Fi loss.
- MQTT transport.
- NMOS IS-07 interoperability.
- Electrical current measurements.
- Long-term reliability.

The test did not measure electrical current or temperature, establish color accuracy across the matrix, or demonstrate thermal safety. Production use requires those checks on the exact installed assembly.
