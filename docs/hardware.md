# Hardware

## Initial target

The supported board is an Adafruit Feather HUZZAH ESP8266 paired with an Adafruit NeoPixel FeatherWing 4×8 RGB. The firmware assumes 32 RGB pixels and an 800 kHz GRB NeoPixel signal.

Adafruit documents the FeatherWing's default control jumper as pin **16** on the ESP8266 Feather, which the Adafruit NeoPixel library does not support on this board. Before using this firmware, cut that default jumper and bridge the **pin 15** jumper as Adafruit suggests for the Feather HUZZAH ESP8266. `TALLY_LED_PIN` consequently defaults to GPIO 15 in `platformio.ini`. Only one control jumper may be closed. Change this compile-time value only when the jumper has been intentionally rerouted and hardware has been checked. The FeatherWing stacks directly with the Feather, so no separate signal wiring is assumed.

Reference: [Adafruit NeoPixel FeatherWing pinouts](https://learn.adafruit.com/adafruit-neopixel-featherwing/pinouts).

## Power and brightness

At full white, 32 NeoPixels can draw substantially more current than USB or a Feather regulator should be assumed to supply. The default brightness is 64/255 to limit the initial load. Use a suitable external 5 V supply for high-brightness or sustained all-pixel use, with common ground. Validate wiring, supply capacity, thermal behavior, and current draw on physical hardware before production use.

Hardware behavior is unverified in this repository. Compilation does not prove the pin mapping, panel behavior, or power safety.
