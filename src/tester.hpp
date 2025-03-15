#pragma once

#include <vector>

#include "actions.hpp"
#include "file.hpp"
#include "rope_editor.hpp"
#include "string.hpp"
#include "types.hpp"

struct Span {
  i64 start, size, input_position;
};

struct Tester {
  String file;
  std::vector<Span> spans;
  i64 current_span_idx = 0;
};

Tester create_tester(String filename)
{
  Tester tester;

  File file;
  if (!read_file(filename, &system_allocator, &file)) {
    exit(1);
  }

  tester.file = file.data;

  std::vector<Span> unordered;
  i64 position = 0;
  while (position < tester.file.size) {
    Span span;
    span.input_position = 0;
    span.start          = position;
    span.size           = std::min(250 + rand() % 500LL, tester.file.size - position);
    unordered.push_back(span);

    position += span.size;
  }

  error("ASHBDGAYSFV ", unordered.size());

  while (unordered.size() > 0) {
    i64 index = std::rand() % unordered.size();
    Span span = unordered[index];
    unordered.erase(unordered.begin() + index);
    for (i64 i = index; i < unordered.size(); i++) {
      unordered[i].input_position += span.size;
    }
    tester.spans.push_back(span);
  }

  return tester;
}

void add_actions(Tester *tester, Five::RopeEditor *editor, Actions *actions)
{
  if (tester->spans.size() == 0) return;

  if (tester->spans.size() == 1) {
    error("ASDA");
  }

  Span &next_span = tester->spans[0];

  String next_string;
  next_string.data = &tester->file[next_span.start];
  next_string.size = next_span.size;

  if (tester->current_span_idx >= next_span.size) {
    tester->spans.erase(tester->spans.begin());
    tester->current_span_idx = 0;
  } else if (editor->cursor.index > next_span.input_position) {
    RopeBuffer::Cursor target = cursor_at(editor->buffer, next_span.input_position);
    if (target.line() != editor->cursor.line()) {
      i64 distance = editor->cursor.line() - target.line();
      while (actions->size < 1000 && distance > 0) {
        actions->push_back(Command::NAV_LINE_UP);
        distance--;
      }
    } else {
      i64 distance = editor->cursor.index - next_span.input_position;
      while (actions->size < 1000 && distance > 0) {
        actions->push_back(Command::NAV_CHAR_LEFT);
        distance--;
      }
    }
  } else if (editor->cursor.index < next_span.input_position) {
    RopeBuffer::Cursor target = cursor_at(editor->buffer, next_span.input_position);
    if (target.line() != editor->cursor.line()) {
      i64 distance = target.line() - editor->cursor.line();
      while (actions->size < 1000 && distance > 0) {
        actions->push_back(Command::NAV_LINE_DOWN);
        distance--;
      }
    } else {
      i64 distance = next_span.input_position - editor->cursor.index;
      while (actions->size < 1000 && distance > 0) {
        actions->push_back(Command::NAV_CHAR_RIGHT);
        distance--;
      }
    }
  } else {
    while (actions->size < 1000 && tester->current_span_idx < next_span.size) {
      actions->push_back(tester->file[next_span.start + tester->current_span_idx]);
      tester->current_span_idx++;
      next_span.input_position++;
    }
  }
}