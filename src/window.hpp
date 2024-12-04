#pragma once

#include <cmath>
#include "actions.hpp"
#include "buffer.hpp"
#include "draw.hpp"
#include "font.hpp"
#include "gpu/metal/texture.hpp"
#include "input.hpp"
#include "platform.hpp"

struct Window {
  BasicBuffer *buffer = nullptr;

  Font font;
  Gpu::Texture atlas_texture;

  f32 size_x, size_y;
  f32 scroll       = 0.f;
  TextPoint cursor = FILE_START;
  TextPoint anchor = FILE_START;
  i64 want_column  = 0.f;
};

Window init_window(Gpu::Device *gpu, Draw::DrawList *dl, BasicBuffer *buffer, i32 size_x,
                   i32 size_y)
{
  Window window;
  window.font = load_font("resources/fonts/jetbrains/JetBrainsMono-Medium.ttf", 24);
  window.atlas_texture = Gpu::create_texture(gpu, window.font.bitmap);
  window.size_x        = size_x;
  window.size_y        = size_y;

  return window;
}

void set_window_buffer(Window *window, BasicBuffer *buffer) { window->buffer = buffer; }

void handle_input(Window *window, Platform::GlfwWindow *platform_window, Input *input,
                  Commands *commands, Commander *commander)
{
  for (i32 i = 0; i < commands->size; i++) {
    Command command = (*commands)[i];

    if (command == Command::BUFFER_PLACE_ANCHOR) {
      window->anchor = window->cursor;
    }

    if (command == Command::BUFFER_COPY) {
      i64 start = std::min(window->cursor.idx, window->anchor.idx);
      i64 end   = std::max(window->cursor.idx, window->anchor.idx);

      String copy_str;
      copy_str.data = window->buffer->data + start;
      copy_str.size = end - start + 1;
      Platform::set_clipboard(platform_window, copy_str);
    }
    if (command == Command::BUFFER_PASTE) {
      String paste_str = Platform::get_clipboard(platform_window);
      for (i32 i = 0; i < paste_str.size; i++) {
        window->cursor = buffer_insert(window->buffer, window->cursor, paste_str.data[i]);
      }
    }

    if (command == Command::NAV_LINE_DOWN) {
      window->cursor =
          find_position(window->buffer, window->cursor.line + 1, window->want_column);
    }
    if (command == Command::NAV_LINE_UP) {
      window->cursor =
          find_position(window->buffer, window->cursor.line - 1, window->want_column);
    }
    if (command == Command::NAV_CHAR_LEFT) {
      window->cursor      = shift_point_backward(window->buffer, window->cursor);
      window->want_column = window->cursor.column;
    }
    if (command == Command::NAV_CHAR_RIGHT) {
      window->cursor      = shift_point_forward(window->buffer, window->cursor);
      window->want_column = window->cursor.column;
    }
    if (command == Command::NAV_WORD_LEFT) {
      bool seen_word = false;
      while (window->cursor.idx > 0) {
        if (!std::isspace(window->buffer->data[window->cursor.idx - 1])) {
          seen_word = true;
        } else if (seen_word) {
          break;
        }
        window->cursor = shift_point_backward(window->buffer, window->cursor);
      }
      window->want_column = window->cursor.column;
    }
    if (command == Command::NAV_WORD_RIGHT) {
      bool seen_word = false;
      while (window->cursor.idx < window->buffer->size) {
        if (!std::isspace(window->buffer->data[window->cursor.idx])) {
          seen_word = true;
        } else if (seen_word) {
          break;
        }
        window->cursor = shift_point_forward(window->buffer, window->cursor);
      }
      window->want_column = window->cursor.column;
    }
  }

  for (i32 i = 0; i < input->key_input.size; i++) {
    Key key = input->key_input[i];

    if (commander->mode == Mode::INSERT) {
      if (key == Key::ENTER) {
        buffer_insert(window->buffer, window->cursor, '\n');
        window->cursor = shift_point_forward(window->buffer, window->cursor);
      }
      if (key == Key::TAB) {
        for (i32 i = 0; i < 2; i++) {
          window->cursor =
              buffer_insert(window->buffer, window->cursor, ' ');
        }
      }
      if (key == Key::BACKSPACE) {
        window->cursor = buffer_remove(window->buffer, window->cursor);
      }
    }
    error("QWER ", window->cursor.idx, ", ", window->cursor.line, ", ",
          window->cursor.column);
  }

  if (commander->mode == Mode::INSERT) {
    for (i32 i = 0; i < input->text_input.size; i++) {
      window->cursor =
          buffer_insert(window->buffer, window->cursor, input->text_input[i]);
      window->want_column = window->cursor.column;
    }
  }

  if (input->mouse_button_up_events[(i32)MouseButton::LEFT]) {
    f32 top_line_of_window = window->scroll;
    f32 space_width        = window->font.glyphs_zero[' '].advance.x;
    i64 clicked_line       = top_line_of_window +
                       (input->mouse_pos.y + window->font.descent) / window->font.height;
    i64 clicked_column  = input->mouse_pos.x / space_width;
    window->cursor      = find_position(window->buffer, clicked_line, clicked_column);
    window->want_column = window->cursor.column;

    error("QWER ", input->mouse_pos.y, ", ", clicked_line, ", ", window->font.size);
  }

  window->scroll -= input->scrollwheel_count;
  window->scroll = fminf(window->scroll, count_lines(window->buffer) - 2);
  window->scroll = fmaxf(window->scroll, 0.f);
}

void draw_window(Window window, Gpu::Device *gpu, Draw::DrawList *dl)
{
  static i32 test = 0;
  test++;

  BasicBuffer *buffer = window.buffer;
  Font &font          = window.font;

  f32 space_width = font.glyphs_zero[' '].advance.x;

  i64 top_line_of_window  = window.scroll;
  i64 num_lines_in_view   = window.size_y / font.height;
  i64 num_columns_in_view = window.size_x / space_width + 1;

  u32 tex_idx = Draw::push_texture(dl, window.atlas_texture);
  Vec2f pos   = {0, -(fmod(window.scroll, 1.f) * font.height)};
  for (i64 i    = find_position(window.buffer, top_line_of_window, 0).idx,
           line = top_line_of_window;
       i < buffer->size && line < top_line_of_window + num_lines_in_view; i++) {
    if (i == window.anchor.idx) {
      Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
      Rect4f border_rect = inset(fill_rect, -1.5f);
      Draw::push_rounded_rect(dl, 0, border_rect, 3, {1.f, 1.f, 1.f, 1.f});
      Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
    }
    if (i == window.cursor.idx) {
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

    Vec4f uv_bounds_zero = {font.glyphs_zero[c].uv.x, font.glyphs_zero[c].uv.y,
                            font.glyphs_zero[c].uv.x + font.glyphs_zero[c].uv.width,
                            font.glyphs_zero[c].uv.y + font.glyphs_zero[c].uv.height};
    Vec4f uv_bounds_one  = {font.glyphs_one[c].uv.x, font.glyphs_one[c].uv.y,
                            font.glyphs_one[c].uv.x + font.glyphs_one[c].uv.width,
                            font.glyphs_one[c].uv.y + font.glyphs_one[c].uv.height};
    Vec4f uv_bounds_two  = {font.glyphs_two[c].uv.x, font.glyphs_two[c].uv.y,
                            font.glyphs_two[c].uv.x + font.glyphs_two[c].uv.width,
                            font.glyphs_two[c].uv.y + font.glyphs_two[c].uv.height};
    Vec4f uvs[3]         = {uv_bounds_zero, uv_bounds_one, uv_bounds_two};

    Color color = (i == window.cursor.idx) ? Color(34, 36, 43) : Color(187, 194, 207);
    push_bitmap_glyph(dl, 0, shape_rect, uvs, color, 0);

    pos.x += glyph.advance.x;
  }

  if (window.anchor.idx == window.buffer->size) {
    Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
    Rect4f border_rect = inset(fill_rect, -1.5f);
    Draw::push_rounded_rect(dl, 0, border_rect, 3, {1.f, 1.f, 1.f, 1.f});
    Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
  }
  if (window.cursor.idx == window.buffer->size) {
    Rect4f cursor_rect = {pos.x, pos.y - font.descent, space_width, font.height};
    Draw::push_rounded_rect(dl, 0, cursor_rect, 1, {67, 158, 254});
  }
}
