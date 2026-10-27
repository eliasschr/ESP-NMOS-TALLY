#include "led/NeoPixelRenderer.h"

#include "config/DeviceConfig.h"

namespace led {

NeoPixelRenderer::NeoPixelRenderer(uint8_t pin, uint8_t count)
    : pixels_(count, pin, NEO_GRB + NEO_KHZ800), available_(false), lastColor_(0), lastBrightness_(0) {}

bool NeoPixelRenderer::begin() {
  pixels_.begin();
  pixels_.clear();
  pixels_.show();
  available_ = true;
  return available_;
}

void NeoPixelRenderer::render(tally::RgbColor color, uint8_t brightness) {
  if (!available_) return;
  brightness = config::clampBrightness(brightness);
  const uint32_t packed = pixels_.Color(color.red, color.green, color.blue);
  if (packed == lastColor_ && brightness == lastBrightness_) return;
  pixels_.setBrightness(brightness);
  pixels_.fill(packed);
  pixels_.show();
  lastColor_ = packed;
  lastBrightness_ = brightness;
}

}  // namespace led
