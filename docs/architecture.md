# Architecture

The firmware keeps policy separate from hardware. `tally::StateMachine` is a small, Arduino-independent state/value component and is tested on the host. `led::NeoPixelRenderer` maps a state color to the 32-pixel FeatherWing. Network connection management and HTTP handling do not write LEDs directly; `Application` coordinates them in its non-blocking loop.

```text
HTTP setup controls -> Application -> Tally State -> NeoPixel renderer
                         |
                         +-> Config store -> LittleFS
Wi-Fi controller --------+
```

`config::DeviceConfig` is a fixed-size, versioned record. The LittleFS store writes a temporary file before replacing the current record. Corrupt, absent, or unsupported records revert safely to defaults and provisioning.

The current interfaces deliberately avoid pretending ESP8266 and ESP32 are interchangeable. Future ESP32 support should supply a platform-specific network, filesystem, and LED implementation while reusing the state/configuration logic after an actual ESP32 build and hardware validation.

The main loop only polls Wi-Fi, HTTP, the startup self-test, and renderer changes. There are no normal-operation delays or reconnect loops. The self-test advances every 300 ms and is therefore watchdog-friendly.
