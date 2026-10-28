# Getting started

1. Stack the NeoPixel FeatherWing on the Feather HUZZAH ESP8266 and connect USB.
2. Build, upload firmware, then upload LittleFS resources:

   ```sh
   pio run -e feather_huzzah_esp8266 -t upload
   pio run -e feather_huzzah_esp8266 -t uploadfs
   ```

   Firmware and the LittleFS dashboard are separate images. Upload both after
   a dashboard or HTTP-form change; uploading only firmware can leave an older
   multipart form cached in LittleFS while newer firmware accepts only the
   URL-encoded form.

   On macOS, the tested Feather's CP2104 USB-to-UART interface appeared after installing the Silicon Labs CP210x VCP driver. If no serial port appears, install the current driver from Silicon Labs, reconnect the board, and select the newly available serial device in PlatformIO. Do not record or publish the device serial number.

3. Open the USB serial console at 115200 baud and enter `PROVISION` on its own line. This is the physical authorization step.
4. The console prints `ESP-NMOS-TALLY-SETUP` and a fresh 24-character provisioning secret. Join that AP with the secret, then open `http://192.168.4.1` before the 10-minute session expires.
5. Enter the same secret in the setup page, then enter a device name, Wi-Fi SSID, and Wi-Fi passphrase. Use a compatible 2.4 GHz Wi-Fi network; the Feather HUZZAH ESP8266 cannot join 5 GHz-only networks. The passphrase is sent only to the device over this local HTTP provisioning page and is not returned by an API or printed in logs. The provisioning secret is held in browser memory only; it is not stored by the page.

6. After saving, wait for the explicit "Configuration saved" acknowledgement. This means LittleFS accepted the configuration, not that Wi-Fi association succeeded. The setup AP may disappear immediately afterward. Reconnect the phone, tablet, or computer to the intended network and open the device dashboard at its DHCP-assigned address. Only its status reporting `wifiState: connected` confirms a connection. If the page says it could not verify connection, that is expected when the provisioning AP has gone away; it is not a claim that the connection failed. This flow was hardware-tested with a phone hotspot on 2026-10-28, but other access points still require their own compatibility test.

The setup page maps common failures to clear messages: check the USB-console secret for authorization failures, wait after too many failures, correct invalid fields, reduce an oversized request, or check LittleFS when saving fails. A timeout or interrupted connection can be normal while the device switches networks. Start a fresh session with `PROVISION` over USB if provisioning expires, the network is incompatible, credentials are rejected, or the device cannot be found on the normal network.

The device tries station Wi-Fi for 20 seconds. Failure uses a non-blocking, bounded burst of three retries with backoff, then a 60-second cooldown before a later burst; it does not start the AP. The read-only status API exposes the fixed connection states `unconfigured`, `provisioning`, `connecting`, `connected`, `cooling_down`, and `failed`, never credentials or broker details. `connected` requires both `WL_CONNECTED` and a non-zero DHCP IPv4 address. Enter `PROVISION` over USB serial at any time to recover or replace credentials without knowing the old network. A valid configuration is stored in LittleFS with a version and CRC-32 integrity check; the previous valid primary is retained as a backup across a successful save. CRC detects accidental corruption, not tampering.

The connected-network dashboard is status-only. To change credentials or use manual test controls, enter `PROVISION` from USB serial. Do not erase LittleFS for routine recovery; reserve a physical filesystem erase for controlled servicing.

## Diagnosing station Wi-Fi

Flash the diagnostic firmware, open USB serial at 115200 baud, and run `WIFI STATUS` before and after provisioning. It reports the saved-configuration presence, setup-AP state, Arduino Wi-Fi status (name and number), connection attempts, last numeric disconnect reason and broad category, DHCP address state, and pending transition state. It deliberately never prints the SSID, passphrase, provisioning secret, MAC address, or configuration record.

After submitting the network form, capture the `WIFI:` serial events for at least one 20-second connection attempt. `station associated` followed by `DHCP completed` distinguishes a successful association and lease; a `station disconnected` event gives the SDK reason needed to investigate authentication or access-point compatibility. `connection timeout` followed by `retry scheduled` shows that the bounded retry policy, rather than an AP restart, is active.

The initial assembly was also reset after a successful save and rejoined without re-entering credentials. It recovered automatically after its tested hotspot was disabled and then re-enabled. These are observed recovery results, not a guarantee of interoperability with a particular router or a long-term reliability result.

## Local LED diagnostics

Use USB serial at 115200 baud to test the actual tally state machine and NeoPixel renderer without network connectivity:

```text
DIAGNOSTICS ON
TALLY PROGRAM
LED TEST
LED RAINBOW
LED BRIGHTNESS 12
LED BRIGHTNESS
LED STOP
EXIT
```

Commands are case-insensitive and must end with Enter. `HELP` lists all commands; `STATUS` reports tally, diagnostic-session, LED-test, and rainbow state only, never stored Wi-Fi credentials or secrets. `PROVISION` retains its existing behavior and is independent of diagnostics.

`DIAGNOSTICS ON` is required before `TALLY PROGRAM`, `TALLY PREVIEW`, `TALLY WARNING`, `TALLY IDENTIFY`, `TALLY OFF`, `LED TEST`, `LED RAINBOW`, `LED STOP`, or `LED BRIGHTNESS`. This explicit local session prevents an accidental serial line from overriding normal control. `EXIT` and `DIAGNOSTICS OFF` cancel an active LED effect, restore the tally state held before diagnostics started, and return to normal application control. The session and its selected tally state are not persisted.

`LED TEST` is non-blocking: red, green, blue, amber, white at the existing 32/255 maximum brightness, then off are each requested for approximately one second. You can type `EXIT` or a `TALLY` command while it is running. Serial output identifies render requests only; inspect the LEDs to confirm physical output.

`LED RAINBOW` starts immediately and continuously moves a full HSV hue gradient across all 32 LEDs at approximately 25 FPS. It uses integer color conversion and `millis()` scheduling, so it does not block network or web handling. `LED STOP`, any `TALLY` command, `LED TEST`, and `EXIT` stop the effect; repeating `LED RAINBOW` restarts it. `LED STOP` also cancels an active `LED TEST`.

`LED BRIGHTNESS` displays the active temporary diagnostic brightness. `LED BRIGHTNESS <value>` accepts only whole values from 0 through 32, rejects invalid input, and immediately redraws the active tally color, test frame, or rainbow frame without restarting an animation. It applies to all diagnostic LED output, is never saved to LittleFS, and is discarded when diagnostics ends; normal application rendering then uses the configured brightness. The renderer always applies the existing 32/255 maximum clamp, including in Wokwi.

The first physical ESP8266 check used a FeatherWing whose data jumper was already routed to GPIO 2. GPIO 2 is also a boot-strapping pin and the Feather's onboard blue LED pin: do not add circuitry that pulls it low at reset, and do not assume another FeatherWing has the same jumper route. The observed result and its limitations are in [hardware test results](hardware-test-results.md).

For an ESP32 browser simulation, follow [Wokwi simulation](wokwi-simulation.md). It uses the same serial command and security model; Wokwi does not make serial input equivalent to a physical access-control guarantee.
