#pragma once

#include <cmath>

#include "actions.hpp"
#include "buffer.hpp"
#include "draw.hpp"
#include "font.hpp"
#include "gpu/metal/texture.hpp"
#include "input.hpp"
#include "platform.hpp"

namespace Five
{

struct Editor {
  BasicBuffer *buffer;

  TextPoint cursor = FILE_START;
  TextPoint anchor = FILE_START;
  i64 want_column  = 0.f;
};

void handle_action(Editor *editor, Action action)
{
  if (action == Command::BUFFER_PLACE_ANCHOR) {
    editor->anchor = editor->cursor;
  }
  // if (action == Command::BUFFER_COPY) {
  //   i64 start = std::min(editor->cursor.idx, editor->anchor.idx);
  //   i64 end   = std::max(editor->cursor.idx, editor->anchor.idx);

  //   String copy_str;
  //   copy_str.data = editor->buffer->data + start;
  //   copy_str.size = end - start + 1;
  //   Platform::set_clipboard(platform_editor, copy_str);
  // }
  // if (action == Command::BUFFER_PASTE) {
  //   String paste_str = Platform::get_clipboard(platform_editor);
  //   for (i32 i = 0; i < paste_str.size; i++) {
  //     editor->cursor = buffer_insert(editor->buffer, editor->cursor, paste_str.data[i]);
  //   }
  // }

  if (action == Command::NAV_LINE_DOWN) {
    editor->cursor =
        find_position(editor->buffer, editor->cursor.line + 1, editor->want_column);
  }
  if (action == Command::NAV_LINE_UP) {
    editor->cursor =
        find_position(editor->buffer, editor->cursor.line - 1, editor->want_column);
  }
  if (action == Command::NAV_CHAR_LEFT) {
    editor->cursor      = shift_point_backward(editor->buffer, editor->cursor);
    editor->want_column = editor->cursor.column;
  }
  if (action == Command::NAV_CHAR_RIGHT) {
    editor->cursor      = shift_point_forward(editor->buffer, editor->cursor);
    editor->want_column = editor->cursor.column;
  }
  if (action == Command::NAV_WORD_LEFT) {
    bool seen_word = false;
    while (editor->cursor.idx > 0) {
      if (!std::isspace(editor->buffer->data[editor->cursor.idx - 1])) {
        seen_word = true;
      } else if (seen_word) {
        break;
      }
      editor->cursor = shift_point_backward(editor->buffer, editor->cursor);
    }
    editor->want_column = editor->cursor.column;
  }
  if (action == Command::NAV_WORD_RIGHT) {
    bool seen_word = false;
    while (editor->cursor.idx < editor->buffer->size) {
      if (!std::isspace(editor->buffer->data[editor->cursor.idx])) {
        seen_word = true;
      } else if (seen_word) {
        break;
      }
      editor->cursor = shift_point_forward(editor->buffer, editor->cursor);
    }
    editor->want_column = editor->cursor.column;
  }

  if (action == Command::INPUT_NEWLINE) {
    buffer_insert(editor->buffer, editor->cursor, '\n');
    editor->cursor = shift_point_forward(editor->buffer, editor->cursor);
  }
  if (action == Command::INPUT_TAB) {
    for (i32 i = 0; i < 2; i++) {
      editor->cursor = buffer_insert(editor->buffer, editor->cursor, ' ');
    }
  }
  if (action == Command::INPUT_BACKSPACE) {
    editor->cursor = buffer_remove(editor->buffer, editor->cursor);
  }
  if (action == Command::INPUT_TEXT) {
      editor->cursor = buffer_insert(editor->buffer, editor->cursor, action.character);
      editor->want_column = editor->cursor.column;
  }

  // TODO: move to parent container
  // if (input->mouse_button_up_events[(i32)MouseButton::LEFT]) {
  // f32 top_line_of_editor = editor->scroll;
  // f32 space_width        = dl->font.glyphs_zero[' '].advance.x;
  // i64 clicked_line       = top_line_of_editor +
  //                    (input->mouse_pos.y + editor->font.descent) / editor->font.height;
  // i64 clicked_column  = input->mouse_pos.x / space_width;
  // editor->cursor      = find_position(editor->buffer, clicked_line, clicked_column);
  // editor->want_column = editor->cursor.column;

  // error("QWER ", input->mouse_pos.y, ", ", clicked_line, ", ", editor->font.size);
  // }

  // TODO: move to parent container
  // editor->scroll -= input->scrollwheel_count;
  // editor->scroll = fminf(editor->scroll, count_lines(editor->buffer) - 2);
  // editor->scroll = fmaxf(editor->scroll, 0.f);
}

// // TODO: move to parent container
// void draw_editor(Editor editor, Gpu::Device *gpu, Draw::List *dl)
// {
//   BasicBuffer *buffer = editor.buffer;
//   Font &font          = dl->font;

//   f32 space_width = font.glyphs_zero[' '].advance.x;

//   i64 top_line_of_editor  = editor.scroll;
//   i64 num_lines_in_view   = editor.size.x / font.height;
//   i64 num_columns_in_view = editor.size.x / space_width + 1;

//   u32 tex_idx = Draw::push_texture(dl, dl->font_texture);
//   Vec2f pos   = {0, -(fmod(editor.scroll, 1.f) * font.height)};
//   for (i64 i    = find_position(&buffer, top_line_of_editor, 0).idx,
//            line = top_line_of_editor;
//        i < buffer->size && line < top_line_of_editor + num_lines_in_view; i++) {
//     if (i == editor.anchor.idx) {
//       Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
//       Rect4f border_rect = inset(fill_rect, -1.5f);
//       Draw::push_rounded_rect(dl, 0, border_rect, 3, {1.f, 1.f, 1.f, 1.f});
//       Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
//     }
//     if (i == editor.cursor.idx) {
//       Rect4f cursor_rect = {pos.x, pos.y - font.descent, space_width, font.height};
//       Draw::push_rounded_rect(dl, 0, cursor_rect, 1, {67, 158, 254});
//     }

//     u8 c = buffer->data[i];

//     if (c == '\n') {
//       pos.y += font.height;
//       pos.x = 0;
//       line++;
//       continue;
//     } else if (c == '\t') {
//       pos.x += 2 * space_width;
//       continue;
//     } else if (c == ' ') {
//       pos.x += space_width;
//       continue;
//     }

//     Glyph glyph = font.glyphs_zero[c];

//     Rect4f shape_rect = {pos.x + glyph.bearing.x, pos.y + font.height -
//     glyph.bearing.y,
//                          glyph.size.x, glyph.size.y};

//     Vec4f uv_bounds = {font.glyphs_zero[c].uv.x, font.glyphs_zero[c].uv.y,
//                        font.glyphs_zero[c].uv.x + font.glyphs_zero[c].uv.width,
//                        font.glyphs_zero[c].uv.y + font.glyphs_zero[c].uv.height};

//     Color color = (i == editor.cursor.idx) ? Color(34, 36, 43) : Color(187, 194, 207);
//     push_bitmap_glyph(dl, 0, shape_rect, uv_bounds, color, 0);

//     pos.x += glyph.advance.x;
//   }

//   if (editor.anchor.idx == buffer->size) {
//     Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
//     Rect4f border_rect = inset(fill_rect, -1.5f);
//     Draw::push_rounded_rect(dl, 0, border_rect, 3, {1.f, 1.f, 1.f, 1.f});
//     Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
//   }
//   if (editor.cursor.idx == buffer->size) {
//     Rect4f cursor_rect = {pos.x, pos.y - font.descent, space_width, font.height};
//     Draw::push_rounded_rect(dl, 0, cursor_rect, 1, {67, 158, 254});
//   }
// }
}  // namespace Five
