#pragma once

#include "buffer_manager.hpp"
#include "draw.hpp"
#include "editor.hpp"
#include "window.hpp"

namespace Five
{

struct Menu {
  Editor editor;
  BasicBuffer buffer;

  StackAllocator alloc = StackAllocator(&system_allocator, 32 * MB);

  bool open    = false;
  i32 selected = 0;
  bool entered = false;
};

void close_menu(Menu *menu)
{
  menu->selected = 0;
  menu->entered  = false;
  menu->open     = false;

  clear_and_reset(&menu->editor);
}

void handle_action(Menu *menu, Action action)
{
  if (action == Command::ESCAPE) {
    close_menu(menu);
    return;
  }
  if (action == Command::INPUT_NEWLINE) {
    menu->entered = true;
    return;
  }
  if (action == Command::NAV_LINE_UP) {
    menu->selected = std::max(menu->selected - 1, 0);
    return;
  }
  if (action == Command::NAV_LINE_DOWN) {
    menu->selected += 1;
    return;
  }

  handle_action(&menu->editor, action);
}

i32 score_match(bool first_letter, bool starting_word, bool is_adjacent,
                i32 adjacent_matches)
{
  i32 score = 1;

  if (first_letter) {
    score += 30;
  }

  if (first_letter && starting_word) {
    score += 30;
  }

  if (is_adjacent) {
    score += 10;
  }

  return score * (1 + adjacent_matches);
}

bool is_starting_word(u32 prev_char, u32 current_char)
{
  return (!std::isalnum(prev_char) && std::isalnum(current_char)) ||
         (std::islower(prev_char) && std::isupper(current_char));
}

void fuzzy_subscore(String query, String text, i32 *score, i32 *adjacent_matches)
{
  if (query.size == 0) {
    *score = 0;
    return;
  }

  for (i64 i = 0; i < text.size; i++) {
    if (std::tolower(text[i]) == std::tolower(query[0])) {
      i32 subscore              = -1000;
      i32 next_adjacent_matches = 0;
      fuzzy_subscore({query.data + 1, query.size - 1},
                     {text.data + i + 1, text.size - (i + 1)}, &subscore,
                     &next_adjacent_matches);

      if (i == 0) {
        *adjacent_matches = 1 + next_adjacent_matches;
      }

      i32 combined_score = score_match(false, is_starting_word(text[i - 1], text[i]),
                                       i == 0, next_adjacent_matches) +
                           subscore;
      *score = std::max(*score, combined_score);
    }
  }
}

i32 fuzzy_score(String query, String text)
{
  if (query.size == 0) {
    return 0;
  }

  int score = 0;
  for (i64 i = 0; i < text.size; i++) {
    if (std::tolower(text[i]) == std::tolower(query[0])) {
      i32 subscore         = -1000;
      i32 adjacent_matches = 0;
      fuzzy_subscore({query.data + 1, query.size - 1},
                     {text.data + i + 1, text.size - (i + 1)}, &subscore,
                     &adjacent_matches);

      bool starting_word = i == 0 ? true : is_starting_word(text[i - 1], text[i]);
      i32 combined_score =
          score_match(true, starting_word, false, adjacent_matches) + subscore;
      score = std::max(score, combined_score);
    }
  }

  return score - text.size;  // prefer shorter filenames
}

void sort(DynamicArray<std::pair<i64, i64>> arr)
{
  i64 looking_for = 0;
  for (i64 i = arr.size - 1; i >= 0; i--) {
    for (i64 j = i; j >= 0; j--) {
      if (arr[j].second == looking_for) {
        auto a = arr[i];
        arr[i] = arr[j];
        arr[j] = a;
        break;
      }

      if (j == 0) {
        looking_for++;
      }
    }
  }
}

void merge_sort(DynamicArray<std::pair<i64, i64>> src,
                DynamicArray<std::pair<i64, i64>> dst, i64 start, i64 end)
{
  if (end - start <= 1) return;

  i64 middle = (start + end) / 2;
  merge_sort(dst, src, start, middle);
  merge_sort(dst, src, middle, end);

  i64 left_cursor  = start;
  i64 right_cursor = middle;
  for (i64 i = start; i < end; i++) {
    if (i < middle && src[left_cursor].second >= src[right_cursor].second ||
        right_cursor >= end) {
      dst[i] = src[left_cursor];
      left_cursor++;
    } else {
      dst[i] = src[right_cursor];
      right_cursor++;
    }
  }
}

void draw_filemenu(Menu *menu, Draw::List *dl, Window *active_window)
{
  static bool init = false;
  if (!init) {
    init = true;

    menu->editor.buffer = &menu->buffer;
  }

  menu->alloc.reset();
  DynamicArray<String> files = Platform::list_files(".", &menu->alloc);
  DynamicArray<std::pair<i64, i64>> scores(&menu->alloc);
  DynamicArray<std::pair<i64, i64>> sorted(&menu->alloc);
  scores.set_capacity(files.size);
  for (i64 i = 0; i < files.size; i++) {
    i32 score = fuzzy_score({menu->buffer.data, menu->buffer.size}, files[i]);
    if (score > 0) {
      scores.push_back({i, score});
      sorted.push_back({i, score});
    }
  }
  merge_sort(scores, sorted, 0, scores.size);

  Font &font = dl->font;

  i32 line_count  = std::min((i32)files.size, 32) + 1;
  f32 line_height = font.height;
  f32 margin      = 3;

  f32 height  = (margin * 2) + (line_height * line_count);
  Rect4f rect = {
      0,
      dl->canvas_size.y - height - line_height,
      dl->canvas_size.x,
      height,
  };
  Draw::push_rect(dl, 0, rect, {25, 27, 32});

  {
    String text = {menu->buffer.data, menu->buffer.size};
    Vec2f pos   = {margin, rect.y + margin};
    draw_string(dl, dl->font, {187, 194, 207}, text, pos);
  }

  for (i64 i = 1; i < line_count && i < sorted.size; i++) {
    String text = files[sorted[i - 1].first];

    if (i - 1 == menu->selected) {
      Rect4f line_rect = {
          0,
          rect.y + margin + i * line_height - font.descent,
          dl->canvas_size.x,
          line_height,
      };

      Draw::push_rect(dl, 0, line_rect, {.5f, .55f, .5f, 1.f});
    }

    Vec2f pos = {margin, rect.y + margin + i * line_height};
    pos = draw_string(dl, dl->font, {187, 194, 207}, text, pos);
    {
      pos.x += 5;
      i32 score = fuzzy_score({menu->buffer.data, menu->buffer.size}, text);
      u8 score_characters[32];
      i32 str_len = snprintf((char *)score_characters, 32, "%i", score);

      String score_str = {score_characters, str_len};
      pos = draw_string(dl, dl->font, {187, 194, 207}, score_str, pos);
    }
  }

  if (menu->entered) {
    menu->entered = false;

    if (files.size > 0 && sorted.size > 0) {
      String filename              = files[(i64)sorted[(i64)menu->selected].first];
      RopeBuffer *buffer          = buffer_manager.get_or_open_buffer(filename);

      open_editor(active_window, buffer);
    }

    close_menu(menu);
  }
}

}  // namespace Five