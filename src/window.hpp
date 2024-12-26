#pragma once

#include <cmath>

#include "actions.hpp"
#include "buffer.hpp"
#include "draw.hpp"
#include "editor.hpp"
#include "font.hpp"
#include "gpu/metal/texture.hpp"
#include "input.hpp"
#include "platform.hpp"

namespace Five
{
struct Window : Focusable {
  Editor editor;

  Vec2f size;
  f32 scroll = 0.f;
};

Window init_window(Gpu::Device *gpu, Draw::List *dl, BasicBuffer *buffer, Vec2f size)
{
  Window window;
  window.size          = size;
  window.editor.buffer = buffer;
  return window;
}

void set_window_buffer(Window *window, BasicBuffer *buffer)
{
  window->editor.buffer = buffer;
}

void handle_action(Window *window, Action action)
{
  if (action == Command::BUFFER_SAVE) {
    write_to_disk(window->editor.buffer);
    return;
  }

  if (action == Command::BUFFER_CHANGE_MODE) {
    static i32 i = 0;
    i++;
    if (window->mode == Mode::INSERT) {
      error("INSERT, ", i);
      window->mode = Mode::NORMAL;
    } else if (window->mode == Mode::NORMAL) {
      error("NORMAL, ", i);
      window->mode = Mode::INSERT;
    }

    if (window->mode != Mode::INSERT) {
      if (action == Command::INPUT_NEWLINE || 
          action == Command::INPUT_TAB ||
          action == Command::INPUT_BACKSPACE ||
          action == Command::INPUT_TEXT) {
          return;
      }
    }

    return;
  }

  handle_action(&window->editor, action);

  // if (input->mouse_button_up_events[(i32)MouseButton::LEFT]) {
  //   f32 top_line_of_window = window->scroll;
  //   f32 space_width        = window->font.glyphs_zero[' '].advance.x;
  //   i64 clicked_line       = top_line_of_window +
  //                      (input->mouse_pos.y + window->font.descent) /
  //                      window->font.height;
  //   i64 clicked_column  = input->mouse_pos.x / space_width;
  //   window->cursor      = find_position(window->buffer, clicked_line, clicked_column);
  //   window->want_column = window->cursor.column;
  // }

  // window->scroll -= input->scrollwheel_count;
  // window->scroll = fminf(window->scroll, count_lines(window->buffer) - 2);
  // window->scroll = fmaxf(window->scroll, 0.f);
}

void draw_window(Window window, Gpu::Device *gpu, Draw::List *dl)
{
  BasicBuffer *buffer = window.editor.buffer;
  Font &font          = dl->font;

  f32 space_width = font.glyphs_zero[' '].advance.x;

  i64 top_line_of_window  = window.scroll;
  i64 num_lines_in_view   = window.size.y / font.height;
  i64 num_columns_in_view = window.size.x / space_width + 1;

  Vec2f pos = {0, -(fmod(window.scroll, 1.f) * font.height)};
  for (i64 i    = find_position(window.editor.buffer, top_line_of_window, 0).idx,
           line = top_line_of_window;
       i < buffer->size && line < top_line_of_window + num_lines_in_view; i++) {
    if (i == window.editor.anchor.idx) {
      Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
      Rect4f border_rect = inset(fill_rect, -1.5f);
      Draw::push_rounded_rect(dl, 0, border_rect, 3, {1.f, 1.f, 1.f, 1.f});
      Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
    }
    if (i == window.editor.cursor.idx) {
      Rect4f cursor_rect = {pos.x, pos.y - font.descent, space_width, font.height};
      Draw::push_rounded_rect(dl, 0, cursor_rect, 1, {67, 158, 254});
    }

    u8 c = buffer->data[i];

    if (c == '\n') {
      pos.y += font.height;
      pos.x = 0;
      line++;
      continue;
    } else if (c == '\t') {
      pos.x += 2 * space_width;
      continue;
    } else if (c == ' ') {
      pos.x += space_width;
      continue;
    }

    Glyph glyph = font.glyphs_zero[c];

    Rect4f shape_rect = {pos.x + glyph.bearing.x, pos.y + font.height - glyph.bearing.y,
                         glyph.size.x, glyph.size.y};

    Vec4f uv_bounds = {font.glyphs_zero[c].uv.x, font.glyphs_zero[c].uv.y,
                       font.glyphs_zero[c].uv.x + font.glyphs_zero[c].uv.width,
                       font.glyphs_zero[c].uv.y + font.glyphs_zero[c].uv.height};

    Color color =
        (i == window.editor.cursor.idx) ? Color(34, 36, 43) : Color(187, 194, 207);
    push_bitmap_glyph(dl, 0, shape_rect, uv_bounds, color, 0);

    pos.x += glyph.advance.x;
  }

  if (window.editor.anchor.idx == window.editor.buffer->size) {
    Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
    Rect4f border_rect = inset(fill_rect, -1.5f);
    Draw::push_rounded_rect(dl, 0, border_rect, 3, {1.f, 1.f, 1.f, 1.f});
    Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
  }
  if (window.editor.cursor.idx == window.editor.buffer->size) {
    Rect4f cursor_rect = {pos.x, pos.y - font.descent, space_width, font.height};
    Draw::push_rounded_rect(dl, 0, cursor_rect, 1, {67, 158, 254});
  }
}
}  // namespace Five