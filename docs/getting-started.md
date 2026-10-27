# Getting started

1. Stack the NeoPixel FeatherWing on the Feather HUZZAH ESP8266 and connect USB.
2. Build, upload firmware, then upload LittleFS resources:

   ```sh
   pio run -e feather_huzzah_esp8266 -t upload
   pio run -e feather_huzzah_esp8266 -t uploadfs
   ```

3. On first boot, join the device AP `ESP-NMOS-TALLY-XXXXXX` and open `http://192.168.4.1`.
4. Its password is `Tally-YYYYYY`, where `YYYYYY` is the six-digit uppercase hexadecimal value of `XXXXXX XOR 5A3C19`. This unique-but-derived password is a provisioning access constraint, not a high-assurance secret; use setup only in a physically controlled area.
5. Enter a device name, Wi-Fi SSID, and Wi-Fi passphrase. The passphrase is sent only to the device over the local HTTP provisioning page and is not returned by any API or printed in logs.

The device tries station Wi-Fi for 20 seconds. Failure returns it to recoverable provisioning mode. A valid configuration is stored in LittleFS. There is no factory reset mechanism yet; erase the LittleFS filesystem with PlatformIO only when you have physical custody of the device. Implementing a guarded physical reset is planned.

The connected-network dashboard is status-only. To change credentials or use manual test controls, intentionally return the device to provisioning mode (currently by a failed station connection or clearing LittleFS under physical control).
