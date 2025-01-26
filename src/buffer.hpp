#pragma once

#include <optional>

#include "file.hpp"
#include "memory.hpp"
#include "string.hpp"
#include "types.hpp"

struct TextPoint {
  i64 line   = 0;
  i64 column = 0;
  i64 idx    = 0;

  bool operator!=(const TextPoint &other)
  {
    return line != other.line || column != other.column || idx != other.idx;
  }
};
const TextPoint FILE_START = {0, 0, 0};

struct BasicBuffer {
  u8 *data                       = nullptr;
  i64 size                       = 0;
  i64 capacity                   = 0;
  std::optional<String> filename = std::nullopt;
};

void validate_idx(BasicBuffer *buffer, i64 idx)
{
  assert(idx >= 0);
  assert(idx <= buffer->size);
}
#define VALIDATE_IDX(idx) validate_idx(buffer, idx)
void debug_validate_text_point(BasicBuffer *buffer, TextPoint point)
{
  assert(point.idx >= 0);
  assert(point.idx <= buffer->size);
}
#define VALIDATE_POINT(point) debug_validate_text_point(buffer, point)


BasicBuffer create_buffer()
{
  BasicBuffer buffer;
  return buffer;
}

BasicBuffer load_buffer(std::optional<String> filename)
{
  BasicBuffer buffer;

  if (filename) {
    buffer.filename = filename->copy(&system_allocator);

    File file;
    if (read_file(filename.value(), &tmp_allocator, &file)) {
      buffer.data = (u8 *)malloc(file.data.size);
      memcpy(buffer.data, file.data.data, file.data.size);
      buffer.size     = file.data.size;
      buffer.capacity = file.data.size;

      return buffer;
    }
  }

  buffer.data     = (u8 *)system_allocator.alloc(1024).data;
  buffer.size     = 0;
  buffer.capacity = 1024;
  return buffer;
}

void write_to_disk(BasicBuffer *buffer)
{
  if (buffer->filename) {
    write_file(buffer->filename.value(), {buffer->data, buffer->size});
  }
}

i64 count_lines(BasicBuffer *buffer)
{
  if (buffer->size == 0) return 0;

  i64 count = 1;
  for (i64 i = 0; i < buffer->size; i++) {
    if (buffer->data[i] == '\n') count++;
  }
  return count;
}

i64 count_column(BasicBuffer *buffer, i64 idx)
{
  VALIDATE_IDX(idx);

  if (idx == 0) return 0;

  i64 column = 1;
  idx--;
  while (idx > 0 && buffer->data[idx] != '\n') {
    column++;
    idx--;
  }

  return column;
}

TextPoint get_point(BasicBuffer &buffer, i64 idx)
{
  TextPoint point;

  for (i64 p = 0; p < buffer.size && p < idx; p++) {
    if (buffer.data[p] == '\n') {
      point.line++;
      point.column = 0;
    } else {
      point.column++;
    }
  }

  return point;
}

TextPoint find_position(BasicBuffer *buffer, i64 line, i64 column)
{
  if (line < 0) return FILE_START;

  TextPoint point;
  while (point.idx < buffer->size) {
    if (point.line == line && point.column == column) {
      break;
    }

    if (buffer->data[point.idx] == '\n') {
      if (point.line == line) {
        break;
      }

      point.line++;
      point.column = 0;
    } else {
      point.column++;
    }

    point.idx++;
  }

  return point;
}

TextPoint buffer_insert(BasicBuffer *buffer, TextPoint point, u8 character)
{
  VALIDATE_POINT(point);

  TextPoint new_point = point;
  new_point.idx++;
  new_point.column++;
  if (character == '\n') {
    new_point.column = 0;
    new_point.line++;
  }

  if (buffer->size <= buffer->capacity) {
    buffer->capacity += 128;
    buffer->data = (u8 *)realloc(buffer->data, buffer->capacity);
  }
  memcpy(buffer->data + point.idx + 1, buffer->data + point.idx,
         buffer->size - point.idx);
  buffer->data[point.idx] = character;
  buffer->size++;

  return new_point;
}

TextPoint buffer_remove(BasicBuffer *buffer, TextPoint point)
{
  VALIDATE_POINT(point);
  if (point.idx == 0) {
    return point;
  }

  TextPoint new_point = point;
  new_point.idx--;
  new_point.column--;
  if (buffer->data[point.idx] == '\n') {
    new_point.column = count_column(buffer, point.idx - 1);
    new_point.line--;
  }

  memcpy(buffer->data + point.idx - 1, buffer->data + point.idx,
         buffer->size - point.idx);
  buffer->size--;

  return new_point;
}

TextPoint shift_point_forward(BasicBuffer *buffer, TextPoint point)
{
  VALIDATE_POINT(point);

  i64 next_idx = point.idx + 1;
  if (next_idx > buffer->size) {
    return point;
  }

  point.column++;
  if (buffer->data[point.idx] == '\n') {
    point.column = 0;
    point.line++;
  }

  point.idx = next_idx;

  return point;
}

TextPoint shift_point_backward(BasicBuffer *buffer, TextPoint point)
{
  VALIDATE_POINT(point);

  i64 next_idx = point.idx - 1;
  if (next_idx < 0) {
    return point;
  }

  point.column--;
  if (buffer->data[next_idx] == '\n') {
    point.column = count_column(buffer, next_idx);
    point.line--;
  }

  point.idx = next_idx;

  return point;
}

String get_line_contents(BasicBuffer *buffer, i64 line) {
  assert(line >= 0);

  TextPoint point = FILE_START;
  while (point.idx < buffer->size && point.line < line) {
    if (buffer->data[point.idx] == '\n') {
      point.line++;
    }
    point.idx++;
  }

  String contents = {&buffer->data[point.idx], 0};
  while (point.idx < buffer->size && buffer->data[point.idx] != '\n') {
    point.idx++;
    contents.size++;
  }
  return contents;
}
