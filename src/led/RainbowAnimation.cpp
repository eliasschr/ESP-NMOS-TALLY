#include "led/RainbowAnimation.h"

namespace led {

RainbowAnimation::RainbowAnimation() : active_(false), startedAt_(0), nextFrameAt_(0), currentHue_(0) {}

void RainbowAnimation::start(uint32_t now) {
  active_ = true;
  startedAt_ = now;
  nextFrameAt_ = now;
  currentHue_ = 0;
}

void RainbowAnimation::stop() { active_ = false; }

bool RainbowAnimation::update(uint32_t now) {
  if (!active_ || static_cast<int32_t>(now - nextFrameAt_) < 0) return false;
  currentHue_ = static_cast<uint8_t>((now - startedAt_) / kHueStepIntervalMs);
  nextFrameAt_ = now + kFrameIntervalMs;
  return true;
}

tally::RgbColor RainbowAnimation::colorForPixel(uint8_t logicalPixel, uint8_t pixelCount) const {
  if (pixelCount == 0) return {0, 0, 0};
  const uint8_t movingHue = static_cast<uint8_t>((static_cast<uint32_t>(logicalPixel) * 256U) /
                                                 pixelCount);
  return hsvToRgb(static_cast<uint8_t>(movingHue + currentHue_));
}

tally::RgbColor RainbowAnimation::hsvToRgb(uint8_t hue) {
  // Multiplying by six divides the 0..255 hue circle into RGB sectors. The
  // fractional component is enough for a smooth full-intensity gradient.
  const uint16_t scaledHue = static_cast<uint16_t>(hue) * 6U;
  const uint8_t sector = scaledHue >> 8;
  const uint8_t fraction = scaledHue & 0xff;
  const uint8_t rising = fraction;
  const uint8_t falling = 255U - fraction;
  switch (sector) {
    case 0: return {255, rising, 0};
    case 1: return {falling, 255, 0};
    case 2: return {0, 255, rising};
    case 3: return {0, falling, 255};
    case 4: return {rising, 0, 255};
    default: return {255, 0, falling};
  }
}

}  // namespace led
