#pragma once

#include "math/color.hpp"
#include "types.hpp"

struct Settings {
  Color text_color = Color(187, 194, 207);

  Color background_color  = Color(40, 44, 52);
  Color solid_color       = Color(34, 36, 43);
  Color foreground_color  = Color(28, 30, 35);
  Color activated_color   = Color(33, 162, 234);
  Color deactivated_color = Color(91, 98, 104);

  // window
  f32 margin          = 12;
  f32 info_bar_height = 32;
  f32 ui_row_height   = 24;

  // editor
  f32 editor_margin        = 6;
  i32 editor_scroll_margin = 3;
};

Settings settings;