#pragma once

#include "draw.hpp"
#include "editor.hpp"
#include "window.hpp"

namespace Five
{
struct Menu : Focusable {
  Editor editor;
  BasicBuffer buffer;

  StackAllocator alloc = StackAllocator(&system_allocator, 32 * MB);

  bool open = false;
  i32 selected = 0;
  bool entered = false;
};

void handle_action(Menu *menu, Action action)
{
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

i32 fuzzy_score(String query, String text)
{
  i32 score = 0;

  if (query.size == 0) return score;

  for (i64 i = 0; i < text.size; i++) {
    if (std::tolower(text[i]) == std::tolower(query[0])) {
      i32 base_score      = 1;
      i32 adjacency_bonus = i == 0 ? 3 : 0;

      i32 subscore = fuzzy_score({query.data + 1, query.size - 1},
                                 {text.data + i + 1, text.size - (i + 1)});

      i32 combined_score = base_score + adjacency_bonus + subscore;
      score              = std::max(score, combined_score);
    }
  }

  return score;
}

void sort(DynamicArray<std::pair<i32, i32>> arr)
{
  i32 looking_for = 0;
  for (i32 i = arr.size - 1; i >= 0; i--) {
    for (i32 j = i; j >= 0; j--) {
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

void merge_sort(DynamicArray<std::pair<i32, i32>> src,
                DynamicArray<std::pair<i32, i32>> dst, i32 start, i32 end)
{
  if (end - start <= 1) return;

  i32 middle = (start + end) / 2;
  merge_sort(dst, src, start, middle);
  merge_sort(dst, src, middle, end);

  i32 left_cursor = start;
  i32 right_cursor = middle;
  for (i32 i = start; i < end; i++) {
    if (i < middle 
        && src[left_cursor].second >= src[right_cursor].second
        || right_cursor >= end){
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
  DynamicArray<std::pair<i32, i32>> scores(&menu->alloc);
  DynamicArray<std::pair<i32, i32>> sorted(&menu->alloc);
  scores.set_capacity(files.size);
  for (i32 i = 0; i < files.size; i++) {
    i32 score = fuzzy_score({menu->buffer.data, menu->buffer.size}, files[i]);
    scores.push_back({i, score});
    sorted.push_back({i, score});
  }
  merge_sort(scores, sorted, 0, scores.size);

  Font &font = dl->font;

  i32 line_count  = std::min(files.size, 32) + 1;
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
    for (i32 i = 0; i < text.size; i++) {
      u8 c        = text[i];
      Glyph glyph = font.glyphs_zero[c];

      Rect4f shape_rect = {pos.x + glyph.bearing.x, pos.y + font.height - glyph.bearing.y,
                           glyph.size.x, glyph.size.y};

      Vec4f uv_bounds = {font.glyphs_zero[c].uv.x, font.glyphs_zero[c].uv.y,
                         font.glyphs_zero[c].uv.x + font.glyphs_zero[c].uv.width,
                         font.glyphs_zero[c].uv.y + font.glyphs_zero[c].uv.height};

      Color color = Color(187, 194, 207);
      push_bitmap_glyph(dl, 0, shape_rect, uv_bounds, color, 0);
      pos.x += glyph.advance.x;
    }
  }

  for (i32 i = 1; i < line_count; i++) {
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
    for (i32 i = 0; i < text.size; i++) {
      u8 c        = text[i];
      Glyph glyph = font.glyphs_zero[c];

      Rect4f shape_rect = {pos.x + glyph.bearing.x, pos.y + font.height - glyph.bearing.y,
                           glyph.size.x, glyph.size.y};

      Vec4f uv_bounds = {font.glyphs_zero[c].uv.x, font.glyphs_zero[c].uv.y,
                         font.glyphs_zero[c].uv.x + font.glyphs_zero[c].uv.width,
                         font.glyphs_zero[c].uv.y + font.glyphs_zero[c].uv.height};

      Color color = Color(187, 194, 207);
      push_bitmap_glyph(dl, 0, shape_rect, uv_bounds, color, 0);
      pos.x += glyph.advance.x;
    }

    // {
    //   i32 score = fuzzy_score({menu->buffer.data, menu->buffer.size}, text);
    //   u8 score_characters[32];
    //   i32 str_len = snprintf((char *)score_characters, 32, "%i", score);

    //   String score_str = {score_characters, str_len};

    //   for (i32 i = 0; i < score_str.size; i++) {
    //     u8 c        = score_str[i];
    //     Glyph glyph = font.glyphs_zero[c];

    //     Rect4f shape_rect = {pos.x + glyph.bearing.x,
    //                          pos.y + font.height - glyph.bearing.y, glyph.size.x,
    //                          glyph.size.y};

    //     Vec4f uv_bounds = {font.glyphs_zero[c].uv.x, font.glyphs_zero[c].uv.y,
    //                        font.glyphs_zero[c].uv.x + font.glyphs_zero[c].uv.width,
    //                        font.glyphs_zero[c].uv.y + font.glyphs_zero[c].uv.height};

    //     Color color = Color(187, 194, 207);
    //     push_bitmap_glyph(dl, 0, shape_rect, uv_bounds, color, 0);
    //     pos.x += glyph.advance.x;
    //   }
    // }
  }

  if (menu->entered) {
    menu->entered = false;

    String filename = files[sorted[menu->selected].first];
    BasicBuffer buffer = load_buffer(filename);
    *(active_window->editor.buffer) = buffer;

    menu->open = false;
  }
}

}  // namespace Five