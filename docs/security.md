# Security

## Trust boundaries

Wi-Fi form fields and all HTTP requests are untrusted. Field lengths are limited before copying into fixed-size configuration storage. The status endpoint never returns the saved Wi-Fi password. Credentials are not logged. Configuration and manual LED controls are available only while the device is in provisioning mode.

The provisioning page requires an in-memory setup token in a custom request header. The browser cannot attach that header cross-origin without an allowed CORS preflight, which provides a basic CSRF barrier. No CORS policy is enabled. This is not user authentication.

## Known limitations

The initial web interface is HTTP-only. Use it only on the device's temporary provisioning AP in a controlled physical environment. The per-device derived AP password prevents a universal factory password but is not cryptographically strong because its input is tied to the device identifier visible in the AP name. It must not be considered protection against a capable nearby attacker.

LittleFS configuration storage is not encrypted. Treat physical flash access as credential exposure. The firmware exposes no remote internet management and stops its AP after joining station Wi-Fi. The connected-network dashboard is intentionally read-only.

Future work includes a physical factory-reset gesture, stronger device-unique provisioning credentials, authenticated management, TLS where memory and deployment constraints allow, and a formal review before MQTT/NMOS integration.
