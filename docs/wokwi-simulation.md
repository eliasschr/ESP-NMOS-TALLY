# Wokwi ESP32 simulation

## Scope and board

The checked-in circuit is `wokwi/diagram.json`. It uses Wokwi's ESP32 DevKit v1 with 4 MiB flash and one WS2812-compatible 4×8 matrix. GPIO 18 connects to `DIN`, 5 V to `VDD`, and ground to `VSS`. The matrix is configured with `rows: 4`, `cols: 8`, `layout: serpentine`, and Wokwi `brightness: "5"`. Current firmware fills every pixel with the active tally color, so orientation does not change observed tally states.

`brightness` is a Wokwi display multiplier for browser visibility. It is independent of the firmware's NeoPixel brightness setting and does not alter the firmware's 32/255 safety cap, physical LED current, or physical brightness limit.

The simulation runs the actual `esp32dev` firmware: it is not a special example, fake frontend, or security-relaxed build. The ESP32 environment uses the same `data/www/index.html`, `app.css`, and `app.js` files as ESP8266.

## Build a complete image

Install PlatformIO Core and run from the repository root:

```sh
./scripts/build-wokwi.sh
```

The script builds `esp32dev`, builds the LittleFS image, reads the `spiffs` offset from `partitions_wokwi.csv`, merges bootloader, partition table, boot metadata, application, and LittleFS with PlatformIO's esptool, and validates the decoded partition table, component regions, byte-for-byte placement, and exact 4 MiB size. The ignored output is:

```text
dist/esp-nmos-tally-wokwi.bin
```

The Wokwi board and partition table both use 4 MiB flash. The script intentionally does not assume that `firmware.bin` contains the dashboard filesystem.

The ESP32 Arduino LittleFS implementation mounts the partition labelled `spiffs`, even though its filesystem format is LittleFS. The Wokwi layout therefore uses the `spiffs` label with the ESP-IDF `data/spiffs` subtype at `0x1d0000`; the generated LittleFS image occupies its complete 0x230000-byte partition. This label is part of the runtime interface, not a filesystem-format choice.

## Upload and run in the browser

1. Create a Wokwi ESP32 custom-firmware project.
2. Replace its circuit with the contents of `wokwi/diagram.json` (or import that file with the Diagram Editor).
3. Open the command palette with **F1**, select **Upload Firmware and Start Simulation**, and select `dist/esp-nmos-tally-wokwi.bin`.
4. Open the Serial Monitor at 115200 baud. Confirm boot and LittleFS initialization, then enter `PROVISION` and confirm that the provisioning AP activation is reported. Do not record the temporary secret shown by this command.
5. Enter `DIAGNOSTICS ON` followed by Enter, then send `TALLY PROGRAM`, `TALLY PREVIEW`, `TALLY WARNING`, `TALLY IDENTIFY`, or `TALLY OFF` to view the real renderer output. Use `LED TEST` to request red, green, blue, amber, white at the 32/255 safety limit, and off for approximately one second each. It remains responsive: `EXIT` cancels the test and restores normal control.
6. The `PROVISION` command starts the normal ten-minute setup session. It prints a temporary secret; diagnostic commands neither expose nor modify stored Wi-Fi credentials.

## Recorded browser runtime result

The first manual Wokwi runtime test used the ESP32 firmware from commit `0a9058e59b7d102b6a68dee2c8c6679eb04e5ac7`.

- The ESP32 booted successfully without recurring watchdog resets or lwIP assertions.
- LittleFS initialized successfully. First-boot missing configuration files were handled correctly.
- The Serial Monitor worked, and the serial provisioning command was processed; provisioning AP activation was confirmed from serial output.
- `DIAGNOSTICS ON`, `LED TEST`, `TALLY PROGRAM`, `TALLY PREVIEW`, `TALLY IDENTIFY`, `TALLY WARNING`, and `TALLY OFF` were exercised.
- The 4×8 matrix and GPIO 18 output were visually confirmed: PROGRAM red, PREVIEW green, IDENTIFY blue, WARNING amber/yellow, and OFF unlit. Serial diagnostics and the LED renderer also passed this manual check.

The full automatic `LED TEST` timing sequence was requested but not independently timed. The result confirms simulated renderer output only, not physical electrical behavior or protocol interoperability.

## Expected LED behavior

| State | Matrix color |
| --- | --- |
| PROGRAM | Red |
| PREVIEW | Green |
| WARNING | Amber |
| IDENTIFY | Blue |
| OFF | Disabled/off |

HTTP manual tally controls remain protected by the real provisioning session, same-origin check, secret header, and authorization cooldown. Serial diagnostics use a separate, explicit `DIAGNOSTICS ON` local session and are not exposed through HTTP. There is no simulator-only route, fixed password, or authentication bypass.

## Limitations and troubleshooting

- The browser simulator's serial console cannot establish physical presence; it only exercises the same code path.
- Browser Wokwi simulations may not make the ESP32 private Wi-Fi address reachable from the host browser. Do not expose the device through a public proxy merely to reach the web UI. Use native tests for shared request validation and test the actual dashboard on a controlled physical network when available.
- If the dashboard reports that resources are unavailable, rebuild with `./scripts/build-wokwi.sh`; it explicitly runs the LittleFS build before merging.
- If Wokwi reports a flash/partition problem, ensure the circuit still declares `flashSize: "4"` and upload the complete merged image rather than `.pio/build/esp32dev/firmware.bin`.
- The browser run did not verify real Wi-Fi access-point connectivity, HTTP dashboard access, browser-based provisioning, MQTT, NMOS IS-07, network recovery, physical ESP8266 hardware, physical ESP32 hardware, or LED current consumption. It is not evidence of full NMOS compliance.
