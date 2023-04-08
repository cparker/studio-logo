#pragma once

#include <FastLED.h>
#include <string>
#include <vector>

struct ColorTransition {
  std::string sectionName;
  CRGB startColor;
  CRGB endColor;
};
