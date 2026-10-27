#pragma once

#include <stdint.h>

#include "tally/TallyState.h"

namespace led {

class Renderer {
 public:
  virtual ~Renderer() = default;
  virtual bool begin() = 0;
  virtual void render(tally::RgbColor color, uint8_t brightness) = 0;
};

}  // namespace led
