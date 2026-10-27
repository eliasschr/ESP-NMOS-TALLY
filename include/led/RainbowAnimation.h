#pragma once

#include <stdint.h>

#include "tally/TallyState.h"

namespace led {

// Generates a full-saturation/value HSV rainbow without floating point. It is
// Arduino-independent so its wrap-safe scheduling and color boundaries can be
// checked by native tests.
class RainbowAnimation {
 public:
  static constexpr uint32_t kFrameIntervalMs = 40;
  static constexpr uint32_t kHueStepIntervalMs = 20;

  RainbowAnimation();

  bool active() const { return active_; }
  void start(uint32_t now);
  void stop();
  // Returns true when a new frame is due. Signed subtraction is intentional:
  // it keeps the deadline comparison correct across millis() rollover.
  bool update(uint32_t now);
  tally::RgbColor colorForPixel(uint8_t logicalPixel, uint8_t pixelCount) const;
  static tally::RgbColor hsvToRgb(uint8_t hue);

 private:
  bool active_;
  uint32_t startedAt_;
  uint32_t nextFrameAt_;
  uint8_t currentHue_;
};

}  // namespace led
