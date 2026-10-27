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

void NeoPixelRenderer::renderPixels(const tally::RgbColor* logicalColors, uint8_t count,
                                    uint8_t brightness) {
  if (!available_ || logicalColors == nullptr || count != pixels_.numPixels()) return;
  brightness = config::clampBrightness(brightness);
  pixels_.setBrightness(brightness);
  for (uint8_t logicalPixel = 0; logicalPixel < count; ++logicalPixel) {
    const tally::RgbColor color = logicalColors[logicalPixel];
    pixels_.setPixelColor(logicalToPhysicalPixel(logicalPixel),
                          pixels_.Color(color.red, color.green, color.blue));
  }
  pixels_.show();
  // A gradient does not have one cacheable color. Invalidate the solid-color
  // cache so normal tally rendering always restores every physical pixel.
  lastColor_ = 0xffffffffUL;
  lastBrightness_ = brightness;
}

uint8_t NeoPixelRenderer::logicalToPhysicalPixel(uint8_t logicalPixel) {
  constexpr uint8_t kColumns = 8;
  const uint8_t row = logicalPixel / kColumns;
  const uint8_t column = logicalPixel % kColumns;
  // The 4x8 matrix is wired in alternating row directions, matching Wokwi's
  // checked-in serpentine layout and keeping the visual gradient continuous.
  return row % 2 == 0 ? logicalPixel : static_cast<uint8_t>(row * kColumns + kColumns - 1 - column);
}

}  // namespace led
