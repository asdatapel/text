#pragma once

#include "editor.hpp"
#include "types.hpp"

namespace Five
{
struct FindPrompt {
  Editor find_input;

  Rect4f rect;
  bool open    = false;
  bool focused = false;
};

bool handle_action(FindPrompt *prompt, Action action)
{
  if (action == Command::TOGGLE_FIND) {
    prompt->focused = false;
    return true;
  }
  if (action == Command::ESCAPE) {
    prompt->open    = false;
    prompt->focused = false;
    return true;
  }

  if (action == Command::MOUSE_LEFT_CLICK) {
    if (!in_rect(action.mouse_position, prompt->rect)) {
      return false;
    }

    // Vec2f position = action.mouse_position - prompt->rect.xy();

    // f32 top_line_of_window = prompt->active_editor->scroll;
    // f32 space_width        = dl->font.glyphs_zero[' '].advance.x;
    // i64 clicked_line =
    //     top_line_of_window + (position.y + dl->font.descent) / dl->font.height;
    // i64 clicked_column = position.x / space_width;
    // prompt->active_editor->cursor =
    //     find_position(prompt->active_editor->buffer, clicked_line, clicked_column);
    // prompt->active_editor->want_column = prompt->active_editor->cursor.column;

    return true;
  }

  handle_action(&prompt->find_input, action);
  return true;
}

void draw_find_prompt(FindPrompt &prompt, Draw::List *dl, bool focused)
{
  Draw::push_rect(dl, 0, prompt.rect, settings.foreground_color);

  ViewRange full_view_range;
  full_view_range.top_line      = 0;
  full_view_range.num_lines     = 1;
  full_view_range.last_line     = 1;
  f32 space_width               = dl->font.glyphs_zero[' '].advance.x;
  full_view_range.num_columns   = prompt.rect.width / space_width + 1;
  full_view_range.text_offset.x = 0;
  full_view_range.text_offset.y = 0;

  draw_editor(prompt.find_input, dl, prompt.rect, full_view_range,
              focused && prompt.focused);
}

}  // namespace Five