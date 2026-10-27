#pragma once

#include <Adafruit_NeoPixel.h>

#include "led/LedRenderer.h"

namespace led {

class NeoPixelRenderer final : public Renderer {
 public:
  NeoPixelRenderer(uint8_t pin, uint8_t count);
  bool begin() override;
  void render(tally::RgbColor color, uint8_t brightness) override;
  void renderPixels(const tally::RgbColor* logicalColors, uint8_t count, uint8_t brightness);

 private:
  static uint8_t logicalToPhysicalPixel(uint8_t logicalPixel);
  Adafruit_NeoPixel pixels_;
  bool available_;
  uint32_t lastColor_;
  uint8_t lastBrightness_;
};

}  // namespace led
