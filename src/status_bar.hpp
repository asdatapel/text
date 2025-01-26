#pragma once

#include "actions.hpp"
#include "draw.hpp"
#include "string.hpp"

Chord history;

void draw_status_bar(Draw::List *dl, Chord *chord, Mode mode)
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
  Vec2f pos   = {12.f, rect.y };
  Color color = Color(187, 194, 207);
  for (i32 i = 0; i < text.size; i++) {
    pos = draw_char(dl, dl->font, color, text[i], pos);
  }

  if (chord->size > 0) {
    history = *chord;
  }
  for (i32 i = 0; i < history.size; i++) {
    pos = draw_char(dl, dl->font, color, ' ', pos);

    if ((history)[i].modifiers.super()) {
      draw_string(dl, dl->font, color, "S-", pos);
    }
    if ((history)[i].modifiers.ctrl()) {
      draw_string(dl, dl->font, color, "C-", pos);
    }
    if ((history)[i].modifiers.alt()) {
      draw_string(dl, dl->font, color, "M-", pos);
    }

    String key_string = KEY_STRINGS[(i32)(history)[i].key];
    pos = draw_string(dl, dl->font, color, key_string, pos);
  }
}
