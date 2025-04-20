#pragma once

#include "actions.hpp"
#include "draw.hpp"
#include "global_state.hpp"
#include "string.hpp"

struct StatusBar {
  Chord chord;
  Command command;
};
StatusBar status_bar;

void show_partial_chord(Chord *chord)
{
  status_bar.chord   = *chord;
  status_bar.command = Command::NONE;
}
void show_completed_chord(Chord *chord, Command command)
{
  status_bar.chord   = *chord;
  status_bar.command = command;
}

void draw_status_bar(Draw::List *dl)
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
  Vec2f pos   = {12.f, rect.y};
  Color color = Color(187, 194, 207);
  for (i32 i = 0; i < text.size; i++) {
    pos = draw_char(dl, dl->font, color, text[i], pos);
  }

  if (status_bar.chord.size > 0) {
    Chord *chord = &status_bar.chord;
    for (i32 i = 0; i < chord->size; i++) {
      pos = draw_char(dl, dl->font, color, ' ', pos);

      if ((*chord)[i].modifiers.super()) {
        draw_string(dl, dl->font, color, "S-", pos);
      }
      if ((*chord)[i].modifiers.ctrl()) {
        draw_string(dl, dl->font, color, "C-", pos);
      }
      if ((*chord)[i].modifiers.alt()) {
        draw_string(dl, dl->font, color, "M-", pos);
      }

      String key_string = KEY_STRINGS[(i32)(*chord)[i].key];
      pos               = draw_string(dl, dl->font, color, key_string, pos);
    }
  }

  if (status_bar.command != Command::NONE) {
    pos                   = draw_char(dl, dl->font, color, ' ', pos);
    String command_string = command_strings[(i32)status_bar.command];
    pos                   = draw_string(dl, dl->font, color, command_string, pos);
  }
}