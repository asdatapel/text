#pragma once

#include <cmath>

#include "actions.hpp"
#include "buffer.hpp"
#include "draw.hpp"
#include "find_prompt.hpp"
#include "font.hpp"
#include "input.hpp"
#include "platform.hpp"
#include "rope_editor.hpp"
#include "settings.hpp"

namespace Five
{
struct Window {
  Rect4f rect;
  Rect4f content_rect;
  Rect4f info_bar_rect;

  HashMap<void *, RopeEditor> editors = HashMap<void *, RopeEditor>(&system_allocator);
  RopeEditor *active_editor           = nullptr;

  FindPrompt find;
};

void draw_info_bar(Window window, Draw::List *dl, bool is_focused)
{
  Draw::push_rect(dl, 0, window.info_bar_rect, settings.foreground_color);

  f32 mode_light_width   = 8;
  Rect4f mode_light_rect = {
      window.info_bar_rect.x,
      window.info_bar_rect.y,
      mode_light_width,
      window.info_bar_rect.height,
  };
  Color mode_light_color =
      is_focused ? settings.activated_color : settings.deactivated_color;
  Draw::push_rect(dl, 0, mode_light_rect, mode_light_color);

  Vec2f cursor = {
      window.info_bar_rect.x + mode_light_width + settings.margin,
      window.info_bar_rect.y + dl->font.descent,
  };
  String filename =
      window.active_editor ? window.active_editor->buffer.filename.value() : "*nothing*";
  cursor = Draw::draw_string(dl, dl->font, settings.text_color, filename, cursor);

  cursor.x += settings.margin;
  String line   = window.active_editor
                      ? StaticString<32>::from_i32(window.active_editor->cursor.line())
                      : "";
  String column = window.active_editor
                      ? StaticString<32>::from_i32(window.active_editor->cursor.column())
                      : "";
  String index = window.active_editor
                      ? StaticString<32>::from_i32(window.active_editor->cursor.index)
                      : "";
  cursor = Draw::draw_string(dl, dl->font, settings.text_color, line, cursor);
  cursor = Draw::draw_string(dl, dl->font, settings.text_color, ":", cursor);
  cursor = Draw::draw_string(dl, dl->font, settings.text_color, column, cursor);
  cursor = Draw::draw_string(dl, dl->font, settings.text_color, " ", cursor);
  cursor = Draw::draw_string(dl, dl->font, settings.text_color, index, cursor);

  cursor.x += settings.margin;
  i32 position_percentage =
      window.active_editor
          ? window.active_editor->scroll / count_lines(window.active_editor->buffer) * 100
          : 0;
  String position_percentage_str = StaticString<4>::from_i32(position_percentage);
  cursor = Draw::draw_string(dl, dl->font, settings.text_color, position_percentage_str,
                             cursor);
  cursor = Draw::draw_char(dl, dl->font, settings.text_color, '%', cursor);
}

Window init_window(Rect4f rect)
{
  f32 info_bar_height = 32;

  Window window;
  window.rect         = rect;
  window.content_rect = Rect4f{
      rect.x,
      rect.y,
      rect.width - settings.margin * 2,
      rect.height,
  };
  window.info_bar_rect = {
      rect.x,
      rect.y + rect.height - info_bar_height,
      rect.width,
      info_bar_height,
  };

  // window.find.find_input.buffer = buffer_manager.create_buffer();
  window.find.rect = {
      rect.x,
      rect.y + rect.height - info_bar_height * 2,
      rect.width,
      info_bar_height,
  };

  return window;
}

ViewRange get_view_range(Window &window, Font &font)
{
  ViewRange view_range;
  view_range.top_line  = window.active_editor->scroll;
  view_range.num_lines = window.content_rect.height / font.height + 1;
  view_range.last_line = view_range.top_line + view_range.num_lines;

  f32 space_width        = font.glyphs_zero[' '].advance.x;
  view_range.num_columns = window.content_rect.width / space_width + 1;

  view_range.text_offset.x = 0;
  view_range.text_offset.y = -fmod(window.active_editor->scroll, 1.f);

  return view_range;
}

void open_editor(Window *window, RopeBuffer *buffer)
{
  window->active_editor = window->editors.get(buffer);
  if (!window->active_editor) {
    RopeEditor new_editor;
    new_editor.buffer     = *buffer;
    new_editor.cursor     = cursor_at_point(*buffer, 0, 0);
    new_editor.anchor     = new_editor.cursor;
    window->active_editor = window->editors.put(buffer, new_editor);
  }
}

void handle_action(Window *window, Action action, Draw::List *dl)
{
  if (!window->active_editor) {
    return;
  }

  if (window->find.focused) {
    if (handle_action(&window->find, action)) {
      return;
    }
  }

  if (action == Command::TOGGLE_FIND) {
    window->find.open    = true;
    window->find.focused = true;
    return;
  }

  if (action == Command::BUFFER_SAVE) {
    write_to_disk(window->active_editor->buffer);
    return;
  }

  if (action == Command::MOUSE_SCROLL) {
    window->active_editor->scroll -= action.scrollwheel_delta;
    window->active_editor->scroll = fminf(window->active_editor->scroll,
                                          count_lines(window->active_editor->buffer) - 2);
    window->active_editor->scroll = fmaxf(window->active_editor->scroll, 0.f);

    return;
  }

  if (action == Command::MOUSE_LEFT_CLICK) {
    if (!in_rect(action.mouse_position, window->content_rect)) {
      return;
    }

    Vec2f position = action.mouse_position - window->content_rect.xy();

    f64 top_line_of_window = window->active_editor->scroll;
    f64 space_width        = dl->font.glyphs_zero[' '].advance.x;
    i64 clicked_line =
        top_line_of_window + (position.y + dl->font.descent) / dl->font.height;
    i64 clicked_column = position.x / space_width;
    window->active_editor->cursor =
        cursor_at_point(window->active_editor->buffer, clicked_line, clicked_column);
    window->active_editor->want_column = window->active_editor->cursor.column();

    return;
  }

  RopeBuffer::Cursor previous_cursor = window->active_editor->cursor;
  handle_action(window->active_editor, action);
  if (window->active_editor->cursor != previous_cursor) {
    ViewRange view_range = get_view_range(*window, dl->font);
    if (window->active_editor->cursor.line() < view_range.top_line + 3) {
      window->active_editor->scroll = window->active_editor->cursor.line() - 3;
    } else if (window->active_editor->cursor.line() > view_range.last_line - 3) {
      window->active_editor->scroll =
          window->active_editor->cursor.line() + 3 - view_range.num_lines;
    }

    window->active_editor->scroll = fminf(window->active_editor->scroll,
                                          count_lines(window->active_editor->buffer) - 2);
    window->active_editor->scroll = fmaxf(window->active_editor->scroll, 0.f);
  }
}

void draw_window(Window &window, Draw::List *dl, bool focused)
{
  // if (window.active_editor && window.find.open &&
  //     window.find.find_input.buffer.size > 0) {
  //   struct Match {
  //     i64 start = 0;
  //     i64 count = 0;
  //   };
  //   DynamicArray<Match> matches = DynamicArray<Match>(&system_allocator);

  //   i64 match_counter = 0;
  //   for (i32 i = 0; i < window.active_editor->buffer.size; i++) {
  //     u8 looking_for = window.find.find_input.buffer.data[match_counter];
  //     if (window.active_editor->buffer.data[i] == looking_for) {
  //       match_counter++;
  //       if (match_counter == window.find.find_input.buffer.size) {
  //         Match match = {
  //             i - (match_counter - 1),
  //             match_counter,
  //         };
  //         matches.push_back(match);
  //         match_counter = 0;
  //       }
  //     } else {
  //       match_counter = 0;
  //     }
  //   }

  // error("------------------------------------------------");
  // f32 width  = dl->font.glyphs_zero[' '].advance.x;
  // f32 height = dl->font.height;
  // for (i32 i = 0; i < matches.size; i++) {
  //   TextPoint start = get_point(*window.active_editor->buffer, matches[i].start);
  //   TextPoint end =
  //       get_point(*window.active_editor->buffer, matches[i].start + matches[i].count);

  //   Rect4f rect = {
  //       width * start.column,
  //       height * (start.line - window.active_editor->scroll) - dl->font.descent,
  //       width * (end.column - start.column),
  //       height * (end.line - start.line + 1),
  //   };
  //   error(rect.x, ", ", rect.y, ", ", rect.width, ", ", rect.height);
  //   Draw::push_rect(dl, 0, rect, {1.f, 0.f, 0.f, .5f});
  // }
  // }

  // separator line
  Draw::push_rect(dl, 0, {window.rect.x, window.rect.y, 1, window.rect.height},
                  settings.deactivated_color);

  if (window.active_editor) {
    draw_editor(*window.active_editor, dl, window.content_rect,
                get_view_range(window, dl->font), focused && !window.find.focused);
  }

  if (window.find.open) {
    draw_find_prompt(window.find, dl, focused);
  }
  draw_info_bar(window, dl, focused);
}
}  // namespace Five
