#pragma once

#include "math/color.hpp"
#include "types.hpp"

struct Settings {
  Color text_color        = Color(187, 194, 207);

  Color foreground_color = Color(28, 30, 35);
  Color activated_color   = Color(33, 162, 234);
  Color deactivated_color   = Color(91, 98, 104);

  f32 margin = 12;
};

Settings settings;