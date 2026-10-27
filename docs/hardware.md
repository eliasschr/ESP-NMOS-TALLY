# Hardware

## Initial target

The supported board is an Adafruit Feather HUZZAH ESP8266 paired with an Adafruit NeoPixel FeatherWing 4×8 RGB. The firmware assumes 32 RGB pixels and an 800 kHz GRB NeoPixel signal.

Adafruit documents the FeatherWing's default control jumper as pin **16** on the ESP8266 Feather, which the Adafruit NeoPixel library does not support on this board. Before using this firmware, cut that default jumper and bridge the **pin 15** jumper as Adafruit suggests for the Feather HUZZAH ESP8266. `TALLY_LED_PIN` consequently defaults to GPIO 15 in `platformio.ini`. Only one control jumper may be closed. Change this compile-time value only when the jumper has been intentionally rerouted and hardware has been checked. The FeatherWing stacks directly with the Feather, so no separate signal wiring is assumed.

Reference: [Adafruit NeoPixel FeatherWing pinouts](https://learn.adafruit.com/adafruit-neopixel-featherwing/pinouts).

## Power and brightness

At full white, 32 NeoPixels can draw substantially more current than USB or the Feather regulator should be assumed to supply. The Feather documentation lists the 3.3 V regulator as 500 mA peak and recommends keeping its load below 250 mA; the FeatherWing powers its pixels from USB or battery through Schottky diodes. Firmware defaults to 16/255 and clamps requests at 32/255, but `setBrightness()` is a rendering scale rather than a verified current limiter. Do not treat it as an electrical safety guarantee.

Use a measured, adequately rated 5 V USB supply for this stacked FeatherWing configuration. Do not add pixels without separately powering them and sharing ground, as Adafruit requires. Measure worst-case supply current, voltage drop, regulator/connector temperature, and boot behavior on the actual installed hardware before production use. No such measurements have been performed for this repository.

GPIO 15 is a boot-strapping pin on ESP8266 and must be low during reset. The FeatherWing jumper change described above has not been physically verified with this firmware. Do not add external pull-ups or change this jumper/pin assignment without confirming reset and boot behavior on the finished assembly.

Hardware behavior is unverified in this repository. Compilation does not prove the pin mapping, panel behavior, or power safety.

## ESP32 DevKit and Wokwi target

`esp32dev` targets a generic 4 MiB ESP32 DevKit. Its LED data output is GPIO 18, not GPIO 15; GPIO 18 is a normal output-capable pin on this target and does not inherit the ESP8266 FeatherWing jumper requirement. The Wokwi circuit uses a 4×8, 32-pixel WS2812-compatible matrix with GPIO 18 to `DIN`, 5 V to `VDD`, and ground to `VSS`. Its `serpentine` layout approximates common matrix wiring. Wokwi's `brightness: "5"` is a simulator display multiplier, independent of firmware NeoPixel brightness and physical current limits. The firmware fills all pixels with one color, so matrix orientation does not affect current tally behavior.

Wokwi browser execution at commit `0a9058e59b7d102b6a68dee2c8c6679eb04e5ac7` visually confirmed the simulated GPIO 18 output and all documented tally colors. No ESP32 DevKit, matrix wiring, signal integrity, current draw, boot behavior, or physical LED panel has been tested.
