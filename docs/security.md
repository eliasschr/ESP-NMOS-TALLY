# Security

## Trust boundaries and provisioning

Wi-Fi form fields and all HTTP requests are untrusted. Field lengths are validated before copying into fixed-size configuration storage; malformed methods and oversized post-parse request bodies are rejected. The setup page uses `application/x-www-form-urlencoded`, not multipart form data, so its maximum accepted fields require at most 405 encoded bytes. The 416-byte body limit preserves a small fixed margin while accommodating percent-encoded UTF-8 input. The network endpoint returns bounded error categories for a malformed form, missing or invalid device name, missing or too-long SSID, and invalid password; it never echoes submitted values. Serial diagnostics report only each field's presence and validity, never its contents. The status endpoint never returns saved Wi-Fi credentials or provisioning secrets. Credentials and secrets are not logged. Configuration and manual LED controls are available only in a serial-authorized provisioning session.

USB serial access is the physical authorization boundary. The `PROVISION` command creates a new 96-bit secret from the ESP8266 SDK RNG and starts a 10-minute WPA2 AP session. The secret appears only on USB serial during that command, is never returned by HTTP, and is cleared when the session ends. Every state-changing request requires it in `X-Provisioning-Secret`; the request must also be same-origin when an `Origin` header is present. No CORS policy is enabled. Repeated authorization failures trigger a 30-second cooldown. The page does not use `localStorage`.

Serial tally diagnostics are deliberately separate from provisioning. `DIAGNOSTICS ON` opens a temporary, RAM-only local session before a serial `TALLY ...` command or `LED TEST` is accepted; `EXIT` or `DIAGNOSTICS OFF` restores the state from before the session. Input is line-delimited, case-insensitive, bounded by a fixed 31-character buffer, and oversized lines are discarded through their terminating newline. These commands neither start an AP nor modify saved configuration, and are not available through HTTP. `STATUS` deliberately omits Wi-Fi credentials and provisioning secrets. Render messages acknowledge a request to the driver rather than claiming a physical LED result.

The ESP32 target keeps these controls unchanged. It obtains each provisioning-secret byte from ESP32 `esp_random()` rather than Arduino `random()` or device-identifying data. As with ESP8266, Arduino Wi-Fi credential persistence is disabled; credentials are stored only by the validated LittleFS configuration record. Wokwi serial access simulates physical USB access for functional testing; it is not a physical-authorization boundary and must not be used to evaluate deployment security.

Before provisioning, ESP32 initializes an unconnected STA interface solely to initialize the TCP/IP task required by the HTTP listener. It does not call `WiFi.begin()`, create an AP, or expose state-changing controls. The AP and its provisioning secret still require the serial `PROVISION` command.

`ESP8266WebServer` parses request bodies before route handlers run. This firmware rejects bodies over 416 bytes in handlers but cannot claim a pre-parser allocation bound with that library. The setup AP is time-limited, serial-authorized, and not automatically exposed following a station failure; replacing the web server is required before claiming strict pre-parse body limits. The provisioning response is sent before the firmware begins its station transition, avoiding an intentional AP teardown before the acknowledgement is emitted.

The 2026-10-28 hardware test confirmed successful provisioning and station recovery, not a security assessment of the phone hotspot or of any router. `WIFI STATUS` is intentionally USB-only and reports state, retry information, disconnect reason, DHCP presence, and the current station address; it never prints SSIDs, passphrases, provisioning secrets, MAC addresses, or configuration records.

## Known limitations

The web interface is HTTP-only. Use it only on the temporary provisioning AP in a physically controlled environment. Anyone able to observe USB serial during a setup session or intercept plaintext HTTP on that AP can obtain the active provisioning secret and configure the device. The AP is not an internet-management interface and is unavailable unless a person with physical serial access authorizes it.

LittleFS configuration storage is not encrypted. Treat physical flash access as credential exposure. CRC-32 detects accidental corruption only; it is not cryptographic authentication. The firmware exposes no remote internet management and stops its AP after joining station Wi-Fi. The connected-network dashboard is intentionally read-only.

Future work includes a physical factory-reset gesture, TLS where memory and deployment constraints allow, strict pre-parse HTTP limits using an appropriate server design, and a formal review before MQTT/NMOS integration.
