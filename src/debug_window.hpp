#pragma once

#include "actions.hpp"
#include "containers/array.hpp"
#include "draw.hpp"
#include "editor.hpp"
#include "find_window.hpp"
#include "rope_buffer.hpp"

struct DebugWindow {
  RopeBuffer *buffer;

  Vec2f mouse_position;
  f64 scroll;

  BasicBuffer find_buffer;
  Editor find_input;
};

DebugWindow init_debug_window(RopeBuffer *buffer)
{
  DebugWindow debug_window;
  debug_window.buffer = buffer;

  debug_window.find_buffer       = create_buffer();
  debug_window.find_input.buffer = &debug_window.find_buffer;
  return debug_window;
}

void handle_action(DebugWindow *window, Action *action)
{
  if (eat(action, Command::MOUSE_SCROLL)) {
    window->mouse_position = action->mouse_position;
    return;
  }

  handle_action(&window->find_input, action);
}

// returns max depth
i64 draw(DebugWindow *window, Draw::List *dl, Node *node, i32 x, i32 y, Summary summary)
{
  RopeBuffer *buffer = window->buffer;
  Rope rope          = buffer->rope;

  i64 width = 32;
  i64 gap   = 8;

  Rect4f rect;
  rect.x      = x;
  rect.y      = y;
  rect.width  = 32;
  rect.height = 32;
  Color color = {.7f, .9f, .2f, 1.f};
  if (node->type == Node::Type::NODE) {
    color = {.2f, .1f, .9f, 1.f};
  }
  if (in_rect(window->mouse_position, rect)) {
    color = lighten(color, .15f);
  }
  Draw::push_rect(dl, 0, rect, color);

  if (node->type == Node::Type::NODE) {
    Draw::push_rect(dl, 0,
                    {rect.x + rect.width / 2 - 3, rect.y + rect.height, 6, (f32)gap},
                    {0, 0, 0, 255});
    i64 depthl =
        draw(window, dl, rope.get(node->children.left), x, y + width + gap, summary);

    Draw::push_rect(
        dl, 0,
        {rect.x + rect.width, rect.y + rect.height + gap + rect.height / 2 - 3,
         (f32)(width + gap) * (depthl), 6},
        {0, 0, 0, 255});
    i64 depthr = draw(window, dl, rope.get(node->children.right),
                      x + (width + gap) * (depthl), y + (width + gap),
                      window->buffer->summarizer.summarize(
                          summary, rope.get(node->children.left)->summary));

    if (in_rect(window->mouse_position, rect)) {
      Color white = {1.f, 1.f, 1.f, 1.f};

      Rect4f background_rect;
      background_rect.x      = window->mouse_position.x;
      background_rect.y      = window->mouse_position.y;
      background_rect.width  = 500;
      background_rect.height = 500;
      Draw::push_rect(dl, 0, background_rect, {0.f, 0.f, 0.f, 1.f});

      f32 text_x   = background_rect.x + .5f;
      Vec2f cursor = {text_x, background_rect.y + 5.f};
      cursor       = Draw::draw_string(dl, dl->font, white, "NODE", cursor);

      cursor.x = text_x;
      cursor.y += dl->font.height;
      String ref_count       = StaticString<32>::from_i32(node->ref_count);
      String depth           = StaticString<32>::from_i32(node->depth);
      String size            = StaticString<32>::from_i32(node->summary.size);
      String newlines        = StaticString<32>::from_i32(node->summary.newlines);
      String last_line_chars = StaticString<32>::from_i32(node->summary.last_line_chars);
      cursor   = Draw::draw_string(dl, dl->font, white, "RefCount: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, ref_count, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Depth: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, depth, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Size: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, size, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Newlines: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, newlines, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "LastLineChars: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, last_line_chars, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;

      String index  = StaticString<32>::from_i32(summary.size);
      String line   = StaticString<32>::from_i32(summary.newlines);
      String column = StaticString<32>::from_i32(summary.last_line_chars);

      cursor   = Draw::draw_string(dl, dl->font, white, "Index: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, index, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Line: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, line, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor = Draw::draw_string(dl, dl->font, white, "Column: ", cursor);
      cursor = Draw::draw_string(dl, dl->font, white, column, cursor);
    }

    return std::max(depthl, depthr + depthl);
  } else {
    if (in_rect(window->mouse_position, rect)) {
      Color white = {1.f, 1.f, 1.f, 1.f};

      Rect4f background_rect;
      background_rect.x      = window->mouse_position.x;
      background_rect.y      = window->mouse_position.y;
      background_rect.width  = 500;
      background_rect.height = 500;
      Draw::push_rect(dl, 0, background_rect, {0.f, 0.f, 0.f, 1.f});

      f32 text_x   = background_rect.x + .5f;
      Vec2f cursor = {text_x, background_rect.y + 5.f};
      cursor       = Draw::draw_string(dl, dl->font, white, "LEAF", cursor);

      cursor.x = text_x;
      cursor.y += dl->font.height;
      String ref_count       = StaticString<32>::from_i32(node->ref_count);
      String depth           = StaticString<32>::from_i32(node->depth);
      String size            = StaticString<32>::from_i32(node->summary.size);
      String newlines        = StaticString<32>::from_i32(node->summary.newlines);
      String last_line_chars = StaticString<32>::from_i32(node->summary.last_line_chars);
      cursor   = Draw::draw_string(dl, dl->font, white, "RefCount: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, ref_count, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Depth: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, depth, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Size: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, size, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Newlines: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, newlines, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "LastLineChars: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, last_line_chars, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;

      String index  = StaticString<32>::from_i32(summary.size);
      String line   = StaticString<32>::from_i32(summary.newlines);
      String column = StaticString<32>::from_i32(summary.last_line_chars);

      cursor   = Draw::draw_string(dl, dl->font, white, "Index: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, index, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor   = Draw::draw_string(dl, dl->font, white, "Line: ", cursor);
      cursor   = Draw::draw_string(dl, dl->font, white, line, cursor);
      cursor.x = text_x;
      cursor.y += dl->font.height;
      cursor = Draw::draw_string(dl, dl->font, white, "Column: ", cursor);
      cursor = Draw::draw_string(dl, dl->font, white, column, cursor);

      cursor.x = text_x;
      cursor.y += dl->font.height;

      String chunk = {&(*window->buffer->text)[node->data.index], node->data.size};
      cursor       = Draw::draw_string(dl, dl->font, white, chunk, cursor);
    }
  }

  return 1;
}

void draw(DebugWindow *window, Draw::List *dl, Rect4f target_rect)
{
  Layout::Button button;
  Layout::Button find_button;
  Array<Rect4f, 40> rects;
  rects.resize(40);
  i32 next_rect_i = 0;
  auto next_rect  = [&]() { return &rects[next_rect_i++]; };

  String find_buffer_string;
  find_buffer_string.data = window->find_input.buffer->data;
  find_buffer_string.size = window->find_input.buffer->size;

  // handle the mouse position here, and interaaction later

  Rect4f ui_rect = target_rect;
  Layout::start_ui(ui_rect, Layout::Direction::RIGHT);

  Layout::TextInput search_input;
  Layout::Button search_button;
  Layout::Button dumb_button;

  Layout::start({Layout::grow(), Layout::fit()}, Layout::Direction::RIGHT, next_rect());
  {
    Layout::place_text_input(Layout::grow(), &window->find_input, &search_input);
    Layout::place_button("Dummmmmmmmb", &dumb_button);
    Layout::place_button("Search", &search_button);
  }
  Layout::end();

  // Layout::start({48, 48}, Layout::Direction::RIGHT, next_rect());
  // Layout::end();

  // Layout::start({Layout::grow(), Layout::fit()}, Layout::Direction::DOWN, next_rect());
  // {
  //   Layout::start({32, 64}, Layout::Direction::RIGHT, next_rect());
  //   Layout::end();
  //   Layout::start({48, 25}, Layout::Direction::RIGHT, next_rect());
  //   Layout::end();

  //   Layout::start({32, 64}, Layout::Direction::RIGHT, next_rect());
  //   Layout::end();

  //   Layout::place_button(find_buffer_string, &find_button);

  //   Layout::place_button("Test Me", &button);
  //   Layout::start({Layout::grow(), Layout::grow(10)}, Layout::Direction::RIGHT,
  //                 next_rect());
  //   Layout::end();
  // }
  // Layout::end();

  // Layout::start({20, 48}, Layout::Direction::RIGHT, next_rect());
  // Layout::end();

  Layout::end_ui();

  // for (i32 i = 0; i < rects.size; i++) {
  //   Draw::push_rounded_rect(dl, 0, rects[i], 7, Color::from_int(0x999999));
  //   Draw::push_rounded_rect(dl, 0, inset(rects[i], 1.5), 7, settings.solid_color);
  // }
  // Layout::draw_button(dl, find_buffer_string, &find_button);
  // Layout::draw_button(dl, "Test Me", &button);
  Layout::draw_text_input(dl, &search_input);
  Layout::draw_button(dl, "Search", &search_button);
  Layout::draw_button(dl, "Dummmmmmmmb", &dumb_button);

  // Color white = {1.f, 1.f, 1.f, 1.f};
  // f32 text_x  = target_rect.x - (f32)window->scroll;
  // String free_nodes_count =
  //     StaticString<32>::from_i32(window->buffer->rope.node_pool->count_free());
  // String capacity =
  // StaticString<32>::from_i32(window->buffer->rope.node_pool->capacity); Vec2f cursor =
  // {text_x, target_rect.y}; cursor          = Draw::draw_string(dl, dl->font, white,
  // "FreeCount: ", cursor); cursor          = Draw::draw_string(dl, dl->font, white,
  // free_nodes_count, cursor); cursor.x        = text_x; cursor.y += dl->font.height;
  // cursor   = Draw::draw_string(dl, dl->font, white, "Capacity: ", cursor);
  // cursor   = Draw::draw_string(dl, dl->font, white, capacity, cursor);
  // cursor.x = text_x;
  // cursor.y += dl->font.height;

  // String builder_size = StaticString<32>::from_i32(window->buffer->rope.builder->size);
  // cursor   = Draw::draw_string(dl, dl->font, white, "Builder Size: ", cursor);
  // cursor   = Draw::draw_string(dl, dl->font, white, builder_size, cursor);
  // cursor.x = text_x;
  // cursor.y += dl->font.height;

  // String builder_capacity =
  // StaticString<32>::from_i32(window->buffer->rope.builder->capacity); cursor   =
  // Draw::draw_string(dl, dl->font, white, "Builder Capacity: ", cursor); cursor   =
  // Draw::draw_string(dl, dl->font, white, builder_capacity, cursor); cursor.x = text_x;
  // cursor.y += dl->font.height;

  // String depth = StaticString<32>::from_i32(
  //     window->buffer->rope.get_or_empty(window->buffer->rope.root)->depth);
  // cursor   = Draw::draw_string(dl, dl->font, white, "Depth: ", cursor);
  // cursor   = Draw::draw_string(dl, dl->font, white, depth, cursor);
  // cursor.x = text_x;
  // cursor.y += dl->font.height;

  // String length = StaticString<32>::from_i32(
  //     window->buffer->rope.get_or_empty(window->buffer->rope.root)->summary.size);
  // cursor   = Draw::draw_string(dl, dl->font, white, "Length: ", cursor);
  // cursor   = Draw::draw_string(dl, dl->font, white, length, cursor);
  // cursor.x = text_x;
  // cursor.y += dl->font.height;

  // draw(window, dl, window->buffer->rope.get_or_empty(window->buffer->rope.root),
  //      cursor.x, cursor.y, {});
}