#pragma once

#include <cmath>

#include "actions.hpp"
#include "containers/rope.hpp"
#include "draw.hpp"
#include "font.hpp"
#include "gpu/metal/texture.hpp"
#include "input.hpp"
#include "platform.hpp"
#include "rope_buffer.hpp"
#include "settings.hpp"

namespace Five
{

struct RopeEditor {
  RopeBuffer buffer;

  RopeBuffer::Cursor cursor = {};
  RopeBuffer::Cursor anchor = {};
  i64 want_column           = 0.f;

  f64 scroll = 0.f;
};

void handle_action(RopeEditor *editor, Action action)
{
  if (action == Command::BUFFER_PLACE_ANCHOR) {
    editor->anchor = editor->cursor;
  }
  // if (action == Command::BUFFER_COPY) {
  //   i64 start = std::min(editor->cursor.index - 1, editor->anchor.index);
  //   i64 end   = std::max(editor->cursor.index - 1, editor->anchor.index);

  //   String copy_str;
  //   copy_str.data = editor->buffer->data + start;
  //   copy_str.size = end - start + 1;
  //   Platform::set_clipboard(copy_str);
  // }
  // if (action == Command::BUFFER_PASTE) {
  //   String paste_str = Platform::get_clipboard();
  //   for (i32 i = 0; i < paste_str.size; i++) {
  //     editor->cursor = buffer_insert(editor->buffer, editor->cursor,
  //     paste_str.data[i]);
  //   }
  // }

  if (action == Command::NAV_LINE_DOWN) {
    editor->cursor =
        cursor_at_point(editor->buffer, editor->cursor.line() + 1, editor->want_column);
  }
  if (action == Command::NAV_LINE_UP) {
    editor->cursor =
        cursor_at_point(editor->buffer, editor->cursor.line() - 1, editor->want_column);
  }
  if (action == Command::NAV_CHAR_LEFT) {
    editor->cursor      = cursor_at(editor->buffer, editor->cursor.index - 1);
    editor->want_column = editor->cursor.column();
  }
  if (action == Command::NAV_CHAR_RIGHT) {
    editor->cursor      = cursor_at(editor->buffer, editor->cursor.index + 1);
    editor->want_column = editor->cursor.column();
  }
  // if (action == Command::NAV_WORD_LEFT) {
  //   bool seen_word = false;
  //   while (editor->cursor.index > 0) {
  //     if (!std::isspace(char_at(editor->cursor))) {
  //       seen_word = true;
  //     } else if (seen_word) {
  //       break;
  //     }
  //     editor->cursor = cursor_at(editor->buffer, editor->cursor.index - 1);
  //   }
  //   editor->want_column = editor->cursor.column();
  // }
  // if (action == Command::NAV_WORD_RIGHT) {
  //   bool seen_word = false;
  //   while (editor->cursor.index < editor->buffer.rope->summary.size) {
  //     if (!std::isspace(char_at(editor->cursor))) {
  //       seen_word = true;
  //     } else if (seen_word) {
  //       break;
  //     }
  //     editor->cursor = cursor_at(editor->buffer, editor->cursor.index + 1);
  //   }
  //   editor->want_column = editor->cursor.column();
  // }
  // if (action == Command::NAV_BLOCK_UP) {
  //   i64 line_length         = 0;
  //   RopeBuffer::Iterator it = move_backward_until(editor->cursor, '\n', &line_length);
  //   it                      = move_backward(it);  // eat '\n'

  //   RopeBuffer::Iterator current = it;
  //   RopeBuffer::Iterator next    = move_backward_until(current, '\n', &line_length);
  //   while (line_length > 0 && next != current) {
  //     next    = move_backward(next);  // eat '\n'
  //     current = next;
  //     next    = move_backward_until(current, '\n', &line_length);
  //   }
  //   editor->cursor = to_cursor(next);
  // }
  // if (action == Command::NAV_BLOCK_DOWN) {
  //   i64 line_length       = 0;
  //   RopeBuffer::Cursor it = move_forward_until(editor->cursor, '\n', &line_length);
  //   it                    = move_forward(it);  // eat '\n'

  //   RopeBuffer::Cursor current = it;
  //   RopeBuffer::Cursor next    = move_forward_until(current, '\n', &line_length);
  //   while (line_length > 0 && next != current) {
  //     next    = move_forward(next);  // eat '\n'
  //     current = next;
  //     next    = move_forward_until(current, '\n', &line_length);
  //   }
  //   editor->cursor = next;
  // }

  if (action == Command::INPUT_NEWLINE) {
    buffer_insert(editor->buffer, editor->cursor, '\n');
    editor->cursor = cursor_at(editor->buffer, editor->cursor.index + 1);
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
    editor->cursor      = buffer_insert(editor->buffer, editor->cursor, action.character);
    editor->want_column = editor->cursor.column();
  }
}

void draw_editor(RopeEditor &editor, Draw::List *dl, Rect4f target_rect,
                 ViewRange view_range, bool focused)
{
  RopeBuffer buffer = editor.buffer;
  Font &font        = dl->font;
  f32 space_width   = font.glyphs_zero[' '].advance.x;

  Vec2f pos = {
      target_rect.x + view_range.text_offset.x * space_width,
      target_rect.y + view_range.text_offset.y * font.height,
  };
  RopeBuffer::Cursor iterator = cursor_at_point(buffer, view_range.top_line, 0);
  while ((is_valid(iterator) || is_eof(editor.buffer, iterator)) &&
         iterator.line() < view_range.last_line) {
    Color cursor_color = focused ? settings.activated_color : settings.deactivated_color;
    if (iterator.index == editor.anchor.index) {
      Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
      Rect4f border_rect = inset(fill_rect, -2.f);
      Draw::push_rounded_rect(dl, 0, border_rect, 3, cursor_color);
      Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
    }
    if (iterator.index == editor.cursor.index) {
      Rect4f cursor_rect = {pos.x, pos.y - font.descent, space_width, font.height};
      Draw::push_rounded_rect(dl, 0, cursor_rect, 1, cursor_color);
    }

    if (iterator.index >= buffer.rope.get_summary_or_empty().size) {
      break;
    }

    u8 c = char_at(buffer, iterator);
    if (c == '\n') {
      pos.y += font.height;
      pos.x = target_rect.x;
    } else if (c == '\t') {
      pos.x += 2 * space_width;
    } else if (c == ' ') {
      pos.x += space_width;
    } else {
      if ((i32)c >= font.glyphs_zero.size) {
        c = 0;
      }
      Color color = (iterator.index == editor.cursor.index) ? Color(34, 36, 43)
                                                            : settings.text_color;
      pos         = Draw::draw_char(dl, dl->font, color, c, pos);
    }

    iterator = cursor_at(buffer, iterator.index + 1);
  }
}

void clear_and_reset(RopeEditor *editor)
{
  // editor->buffer->size = 0;
  // editor->cursor       = FILE_START;
  // editor->anchor       = FILE_START;
}
}  // namespace Five
