#pragma once

#include "actions.hpp"
#include "draw.hpp"

void draw_status_bar(Draw::List *dl, Mode mode)
{
  Font &font = dl->font;

  f32 height  = 40.f;
  Rect4f rect = {
      0,
      dl->canvas_size.y - height,
      dl->canvas_size.x,
      height,
  };
  Draw::push_rect(dl, 0, rect, {25, 27, 32});

  String text = mode == Mode::NORMAL ? "*NORMAL*" : "*INSERT*";
  Vec2f pos   = {3.f, rect.y + 3.f};
  for (i32 i = 0; i < text.size; i++) {
    u8 c        = text[i];
    Glyph glyph = font.glyphs_zero[c];

    Rect4f shape_rect = {pos.x + glyph.bearing.x, pos.y + font.height - glyph.bearing.y,
                         glyph.size.x, glyph.size.y};

    Vec4f uv_bounds = {font.glyphs_zero[c].uv.x, font.glyphs_zero[c].uv.y,
                       font.glyphs_zero[c].uv.x + font.glyphs_zero[c].uv.width,
                       font.glyphs_zero[c].uv.y + font.glyphs_zero[c].uv.height};

    Color color = Color(187, 194, 207);
    push_bitmap_glyph(dl, 0, shape_rect, uv_bounds, color, 0);
    pos.x += glyph.advance.x;
  }
}
