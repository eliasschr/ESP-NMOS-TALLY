# Getting started

1. Stack the NeoPixel FeatherWing on the Feather HUZZAH ESP8266 and connect USB.
2. Build, upload firmware, then upload LittleFS resources:

   ```sh
   pio run -e feather_huzzah_esp8266 -t upload
   pio run -e feather_huzzah_esp8266 -t uploadfs
   ```

3. Open the USB serial console at 115200 baud and enter `PROVISION` on its own line. This is the physical authorization step.
4. The console prints `ESP-NMOS-TALLY-SETUP` and a fresh 24-character provisioning secret. Join that AP with the secret, then open `http://192.168.4.1` before the 10-minute session expires.
5. Enter the same secret in the setup page, then enter a device name, Wi-Fi SSID, and Wi-Fi passphrase. The passphrase is sent only to the device over this local HTTP provisioning page and is not returned by an API or printed in logs. The provisioning secret is held in browser memory only; it is not stored by the page.

The device tries station Wi-Fi for 20 seconds. Failure uses a non-blocking, bounded burst of three retries with backoff, then a 60-second cooldown before a later burst; it does not start the AP. Enter `PROVISION` over USB serial at any time to recover or replace credentials without knowing the old network. A valid configuration is stored in LittleFS with a version and CRC-32 integrity check; the previous valid primary is retained as a backup across a successful save. CRC detects accidental corruption, not tampering.

The connected-network dashboard is status-only. To change credentials or use manual test controls, enter `PROVISION` from USB serial. Do not erase LittleFS for routine recovery; reserve a physical filesystem erase for controlled servicing.
