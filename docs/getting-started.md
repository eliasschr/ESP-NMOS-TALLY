# Getting started

1. Stack the NeoPixel FeatherWing on the Feather HUZZAH ESP8266 and connect USB.
2. Build, upload firmware, then upload LittleFS resources:

   ```sh
   pio run -e feather_huzzah_esp8266 -t upload
   pio run -e feather_huzzah_esp8266 -t uploadfs
   ```

   On macOS, the tested Feather's CP2104 USB-to-UART interface appeared after installing the Silicon Labs CP210x VCP driver. If no serial port appears, install the current driver from Silicon Labs, reconnect the board, and select the newly available serial device in PlatformIO. Do not record or publish the device serial number.

3. Open the USB serial console at 115200 baud and enter `PROVISION` on its own line. This is the physical authorization step.
4. The console prints `ESP-NMOS-TALLY-SETUP` and a fresh 24-character provisioning secret. Join that AP with the secret, then open `http://192.168.4.1` before the 10-minute session expires.
5. Enter the same secret in the setup page, then enter a device name, Wi-Fi SSID, and Wi-Fi passphrase. The passphrase is sent only to the device over this local HTTP provisioning page and is not returned by an API or printed in logs. The provisioning secret is held in browser memory only; it is not stored by the page.

The device tries station Wi-Fi for 20 seconds. Failure uses a non-blocking, bounded burst of three retries with backoff, then a 60-second cooldown before a later burst; it does not start the AP. Enter `PROVISION` over USB serial at any time to recover or replace credentials without knowing the old network. A valid configuration is stored in LittleFS with a version and CRC-32 integrity check; the previous valid primary is retained as a backup across a successful save. CRC detects accidental corruption, not tampering.

The connected-network dashboard is status-only. To change credentials or use manual test controls, enter `PROVISION` from USB serial. Do not erase LittleFS for routine recovery; reserve a physical filesystem erase for controlled servicing.

## Local LED diagnostics

Use USB serial at 115200 baud to test the actual tally state machine and NeoPixel renderer without network connectivity:

```text
DIAGNOSTICS ON
TALLY PROGRAM
LED TEST
LED RAINBOW
LED STOP
EXIT
```

Commands are case-insensitive and must end with Enter. `HELP` lists all commands; `STATUS` reports tally, diagnostic-session, LED-test, and rainbow state only, never stored Wi-Fi credentials or secrets. `PROVISION` retains its existing behavior and is independent of diagnostics.

`DIAGNOSTICS ON` is required before `TALLY PROGRAM`, `TALLY PREVIEW`, `TALLY WARNING`, `TALLY IDENTIFY`, `TALLY OFF`, `LED TEST`, `LED RAINBOW`, or `LED STOP`. This explicit local session prevents an accidental serial line from overriding normal control. `EXIT` and `DIAGNOSTICS OFF` cancel an active LED effect, restore the tally state held before diagnostics started, and return to normal application control. The session and its selected tally state are not persisted.

`LED TEST` is non-blocking: red, green, blue, amber, white at the existing 32/255 maximum brightness, then off are each requested for approximately one second. You can type `EXIT` or a `TALLY` command while it is running. Serial output identifies render requests only; inspect the LEDs to confirm physical output.

`LED RAINBOW` starts immediately and continuously moves a full HSV hue gradient across all 32 LEDs at approximately 25 FPS. It uses integer color conversion and `millis()` scheduling, so it does not block network or web handling. `LED STOP`, any `TALLY` command, `LED TEST`, and `EXIT` stop the effect; repeating `LED RAINBOW` restarts it. `LED STOP` also cancels an active `LED TEST`. The effect uses the existing 32/255 physical brightness limit regardless of the saved dashboard brightness and does not change that saved setting.

The first physical ESP8266 check used a FeatherWing whose data jumper was already routed to GPIO 2. GPIO 2 is also a boot-strapping pin and the Feather's onboard blue LED pin: do not add circuitry that pulls it low at reset, and do not assume another FeatherWing has the same jumper route. The observed result and its limitations are in [hardware test results](hardware-test-results.md).

For an ESP32 browser simulation, follow [Wokwi simulation](wokwi-simulation.md). It uses the same serial command and security model; Wokwi does not make serial input equivalent to a physical access-control guarantee.
