#pragma once

#include <Adafruit_NeoPixel.h>

#include "led/LedRenderer.h"

namespace led {

class NeoPixelRenderer final : public Renderer {
 public:
  NeoPixelRenderer(uint8_t pin, uint8_t count);
  bool begin() override;
  void render(tally::RgbColor color, uint8_t brightness) override;

 private:
  Adafruit_NeoPixel pixels_;
  bool available_;
  uint32_t lastColor_;
  uint8_t lastBrightness_;
};

}  // namespace led
