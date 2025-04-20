#pragma once

#include <cmath>

#include "actions.hpp"
#include "buffer.hpp"
#include "containers/rope.hpp"
#include "draw.hpp"
#include "font.hpp"
#include "gpu/metal/texture.hpp"
#include "input.hpp"
#include "platform.hpp"
#include "settings.hpp"

struct Editor {
  BasicBuffer *buffer;

  TextPoint cursor = FILE_START;
  TextPoint anchor = FILE_START;
  i64 want_column  = 0.f;

  f32 scroll = 0.f;
};

void handle_action(Editor *editor, Action *action)
{
  if (eat(action, Command::BUFFER_CHANGE_MODE)) {
    if (mode == Mode::INSERT) {
      mode = Mode::NORMAL;
    } else if (mode == Mode::NORMAL) {
      mode = Mode::INSERT;
    }
  }
  if (eat(action, Command::BUFFER_PLACE_ANCHOR)) {
    editor->anchor = editor->cursor;
  }
  if (eat(action, Command::BUFFER_COPY)) {
    i64 start = std::min(editor->cursor.index - 1, editor->anchor.index);
    i64 end   = std::max(editor->cursor.index - 1, editor->anchor.index);

    String copy_str;
    copy_str.data = editor->buffer->data + start;
    copy_str.size = end - start + 1;
    Platform::set_clipboard(copy_str);
  }
  if (eat(action, Command::BUFFER_PASTE)) {
    String paste_str = Platform::get_clipboard();
    for (i32 i = 0; i < paste_str.size; i++) {
      editor->cursor = buffer_insert(editor->buffer, editor->cursor, paste_str.data[i]);
    }
  }

  if (eat(action, Command::NAV_LINE_DOWN)) {
    editor->cursor =
        find_position(editor->buffer, editor->cursor.line + 1, editor->want_column);
  }
  if (eat(action, Command::NAV_LINE_UP)) {
    editor->cursor =
        find_position(editor->buffer, editor->cursor.line - 1, editor->want_column);
  }
  if (eat(action, Command::NAV_CHAR_LEFT)) {
    editor->cursor      = shift_point_backward(editor->buffer, editor->cursor);
    editor->want_column = editor->cursor.column;
  }
  if (eat(action, Command::NAV_CHAR_RIGHT)) {
    editor->cursor      = shift_point_forward(editor->buffer, editor->cursor);
    editor->want_column = editor->cursor.column;
  }
  if (eat(action, Command::NAV_WORD_LEFT)) {
    bool seen_word = false;
    while (editor->cursor.index > 0) {
      if (!std::isspace(editor->buffer->data[editor->cursor.index - 1])) {
        seen_word = true;
      } else if (seen_word) {
        break;
      }
      editor->cursor = shift_point_backward(editor->buffer, editor->cursor);
    }
    editor->want_column = editor->cursor.column;
  }
  if (eat(action, Command::NAV_WORD_RIGHT)) {
    bool seen_word = false;
    while (editor->cursor.index < editor->buffer->size) {
      if (!std::isspace(editor->buffer->data[editor->cursor.index])) {
        seen_word = true;
      } else if (seen_word) {
        break;
      }
      editor->cursor = shift_point_forward(editor->buffer, editor->cursor);
    }
    editor->want_column = editor->cursor.column;
  }
  if (eat(action, Command::NAV_BLOCK_UP)) {
    i64 line = editor->cursor.line - 1;
    while (line >= 0 && !is_only_whitespace(get_line_contents(editor->buffer, line))) {
      line--;
    }
    editor->cursor = find_position(editor->buffer, line, editor->want_column);
  }
  if (eat(action, Command::NAV_BLOCK_DOWN)) {
    i64 line = editor->cursor.line + 1;
    while (line < count_lines(editor->buffer) &&
           !is_only_whitespace(get_line_contents(editor->buffer, line))) {
      line++;
    }
    editor->cursor = find_position(editor->buffer, line, editor->want_column);
  }

  if (eat(action, Command::INPUT_NEWLINE)) {
    buffer_insert(editor->buffer, editor->cursor, '\n');
    editor->cursor = shift_point_forward(editor->buffer, editor->cursor);
  }
  if (eat(action, Command::INPUT_TAB)) {
    for (i32 i = 0; i < 2; i++) {
      editor->cursor = buffer_insert(editor->buffer, editor->cursor, ' ');
    }
  }
  if (eat(action, Command::INPUT_BACKSPACE)) {
    editor->cursor = buffer_remove(editor->buffer, editor->cursor);
  }
  if (eat(action, Command::INPUT_TEXT)) {
    editor->cursor      = buffer_insert(editor->buffer, editor->cursor, action->character);
    editor->want_column = editor->cursor.column;
  }
}

struct ViewRange {
  i64 top_line;
  i64 num_lines;
  i64 last_line;
  i64 num_columns;
  Vec2f text_offset;
};
void draw_editor(Editor &editor, Draw::List *dl, Rect4f target_rect, ViewRange view_range,
                 bool focused)
{
  BasicBuffer *buffer = editor.buffer;
  Font &font          = dl->font;
  f32 space_width     = font.glyphs_zero[' '].advance.x;

  Vec2f pos = {
      target_rect.x + view_range.text_offset.x * space_width,
      target_rect.y + view_range.text_offset.y * font.height,
  };
  i64 idx  = find_position(buffer, view_range.top_line, 0).index;
  i64 line = view_range.top_line;
  while (line <= view_range.last_line) {
    Color cursor_color = focused ? settings.activated_color : settings.deactivated_color;
    if (idx == editor.anchor.index) {
      Rect4f fill_rect   = {pos.x, pos.y - font.descent, space_width, font.height};
      Rect4f border_rect = inset(fill_rect, -2.f);
      Draw::push_rounded_rect(dl, 0, border_rect, 3, cursor_color);
      Draw::push_rounded_rect(dl, 0, fill_rect, 3, Color(40, 44, 52));
    }
    if (idx == editor.cursor.index) {
      Rect4f cursor_rect = {pos.x, pos.y - font.descent, space_width, font.height};
      Draw::push_rounded_rect(dl, 0, cursor_rect, 1, cursor_color);
    }

    if (idx >= buffer->size) {
      break;
    }

    u8 c = buffer->data[idx];
    if (c == '\n') {
      pos.y += font.height;
      pos.x = target_rect.x;
      line++;
    } else if (c == '\t') {
      pos.x += 2 * space_width;
    } else if (c == ' ') {
      pos.x += space_width;
    } else {
      if ((i32)c >= font.glyphs_zero.size) {
        c = 0;
      }
      Color color =
          (idx == editor.cursor.index) ? Color(34, 36, 43) : settings.text_color;
      pos = Draw::draw_char(dl, dl->font, color, c, pos);
    }

    idx++;
  }
}

void clear_and_reset(Editor *editor)
{
  editor->buffer->size = 0;
  editor->cursor       = FILE_START;
  editor->anchor       = FILE_START;
}
