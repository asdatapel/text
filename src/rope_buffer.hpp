#pragma once

#include <optional>

#include "buffer.hpp"
#include "containers/rope.hpp"
#include "file.hpp"
#include "memory.hpp"
#include "string.hpp"
#include "types.hpp"

void accumulate(Summary *summary, u8 c)
{
  summary->size++;
  if (c == '\n') {
    summary->newlines++;
    summary->last_line_chars = 0;
  } else {
    summary->last_line_chars++;
  }
}
void accumulate_eof(Summary *summary)
{
  summary->size++;
  summary->last_line_chars++;
}
Summary summarize(const Chunk &chunk)
{
  Summary summary;
  for (i64 i = 0; i < chunk.size; i++) {
    accumulate(&summary, chunk.data[i]);
  }
  return summary;
}
Summary summarize(const Summary &left, const Summary &right)
{
  Summary summary;
  summary.size     = left.size + right.size;
  summary.newlines = left.newlines + right.newlines;

  if (right.newlines > 0) {
    summary.last_line_chars = right.last_line_chars;
  } else {
    summary.last_line_chars = left.last_line_chars + right.last_line_chars;
  }

  return summary;
}
Summary summarize(const Chunk &right, i64 index)
{
  Chunk partial_chunk = {right.data, index};
  return summarize(partial_chunk);
}

//////////////////////////////////////////////

struct RopeBuffer {
  struct Iterator {
    NodeRef current = NodeRef::invalid();
    i64 index       = 0;
    i32 node_index  = 0;
    bool operator!=(const Iterator &other)
    {
      return current != other.current || node_index != other.node_index;
    }
  };
  struct Cursor : Iterator {
    Cursor() {}
    Cursor(Iterator it) : Iterator(it) {}

    Summary summary;
    i64 line() { return summary.newlines; }
    i64 column() { return summary.last_line_chars; }
  };

  Rope rope;

  RopeBuffer::Iterator last_edit;
  NodeRef editing_leaf = NodeRef::invalid();

  std::optional<String> filename = std::nullopt;
};

void validate_idx(RopeBuffer buffer, i64 idx)
{
  assert(idx >= 0);
  assert(idx <= buffer.rope.get_summary_or_empty().size);
}
#define VALIDATE_IDX(idx) validate_idx(buffer, idx)
void debug_validate_cursor(RopeBuffer buffer, RopeBuffer::Cursor cursor)
{
  assert(cursor.summary.size >= 0);
  assert(cursor.summary.size <= buffer.rope.get_summary_or_empty().size);
  assert(cursor.current.is_valid());
  assert(cursor.node_index >= 0);

  Node *current_val = buffer.rope.get(cursor.current);
  assert(current_val->type == Node::Type::LEAF);
  assert(cursor.node_index <= current_val->data.size);
}
#define VALIDATE_CURSOR(cursor) debug_validate_cursor(buffer, cursor)

RopeBuffer create_rope_buffer()
{
  RopeBuffer buffer;
  return buffer;
}

RopeBuffer load_rope_buffer(std::optional<String> filename)
{
  RopeBuffer buffer;

  if (filename) {
    buffer.filename = filename->copy(&system_allocator);

    File file;
    if (read_file(filename.value(), &tmp_allocator, &file)) {
      buffer.rope = rope_of(file.data);
      return buffer;
    }
  }

  buffer.rope = rope_of("");
  return buffer;
}

// RopeBuffer::Cursor to_cursor(RopeBuffer::Iterator it)
// {
//   if (!it.current) {
//     return {};
//   }
//   assert(it.current->type == Node::Type::LEAF);

//   Summary summary = summarize(it.current->data, it.node_index);

//   Node *current = it.current;
//   Node *parent  = it.current->parent;
//   while (parent) {
//     if (index_of_child(parent, current) == 1) {
//       summary = summarize(parent->children[0]->summary, summary);
//     }
//     current = parent;
//     parent  = current->parent;
//   }

//   RopeBuffer::Cursor cursor = it;
//   cursor.summary            = summary;
//   return cursor;
// }

i64 count_lines(RopeBuffer buffer)
{
  return buffer.rope.get_summary_or_empty().newlines + 1;
}

u8 char_at(RopeBuffer buffer, RopeBuffer::Iterator it)
{
  Node *current_val = buffer.rope.get(it.current);
  return current_val->data.data[it.node_index];
}

bool is_valid(RopeBuffer buffer, RopeBuffer::Iterator it)
{
  Node *current_val = buffer.rope.get(it.current);
  return it.current.is_valid() && it.node_index >= 0 &&
         it.node_index < current_val->data.size;
}
bool is_valid(RopeBuffer::Cursor c) { return c.current.is_valid(); }
bool is_eof(RopeBuffer buffer, RopeBuffer::Cursor c)
{
  return c.current.is_valid() && c.index == buffer.rope.get_summary_or_empty().size;
}

RopeBuffer::Cursor cursor_at(RopeBuffer buffer, NodeRef root, i64 index,
                             Summary accumulator)
{
  Node *root_val = buffer.rope.get(root);
  if (root_val->type == Node::Type::LEAF) {
    Summary summary = summarize(accumulator, summarize(root_val->data, index));

    RopeBuffer::Cursor cursor;
    cursor.current    = root;
    cursor.index      = summary.size;
    cursor.node_index = index;
    cursor.summary    = summary;
    return cursor;
  }

  Node *left    = buffer.rope.get(root_val->children.left);
  i64 left_size = left->summary.size;
  if (index < left_size) {
    return cursor_at(buffer, root_val->children.left, index, accumulator);
  }
  return cursor_at(buffer, root_val->children.right, index - left_size,
                   summarize(accumulator, left->summary));
}
RopeBuffer::Cursor cursor_at(RopeBuffer buffer, i64 index)
{
  if (!buffer.rope.root.is_valid()) {
    RopeBuffer::Cursor cursor;
    cursor.current    = NodeRef::invalid();
    cursor.index      = 0;
    cursor.node_index = 0;
    cursor.summary    = {};
    return cursor;
  }
  index = std::clamp(index, 0ll, buffer.rope.get_summary_or_empty().size);
  return cursor_at(buffer, buffer.rope.root, index, {});
}

RopeBuffer::Cursor cursor_at_point(RopeBuffer buffer, NodeRef root, i64 want_line,
                                   i64 want_column, Summary accumulator)
{
  Node *root_val = buffer.rope.get(root);
  if (root_val->type == Node::Type::LEAF) {
    i64 line   = 0;
    i64 column = 0;
    i64 index  = 0;
    while (index < root_val->data.size) {
      if (line == want_line && column == want_column) {
        break;
      }

      if (root_val->data.data[index] == '\n') {
        if (line == want_line) {
          break;
        }
        line++;
        column = -1;
      }

      column++;
      index++;
    }

    Summary summary = summarize(accumulator, summarize(root_val->data, index));

    RopeBuffer::Cursor cursor;
    cursor.current    = root;
    cursor.index      = summary.size;
    cursor.node_index = index;
    cursor.summary    = summary;
    return cursor;
  }

  Node *left       = buffer.rope.get(root_val->children.left);
  i64 left_lines   = left->summary.newlines;
  i64 left_columns = left->summary.last_line_chars;
  if (want_line == left_lines && want_column >= left_columns) {
    accumulator = summarize(accumulator, left->summary);
    return cursor_at_point(buffer, root_val->children.right, want_line - left_lines,
                           want_column - left_columns, accumulator);
  }
  if (want_line <= left_lines) {
    return cursor_at_point(buffer, root_val->children.left, want_line, want_column,
                           accumulator);
  }
  accumulator = summarize(accumulator, left->summary);
  return cursor_at_point(buffer, root_val->children.right, want_line - left_lines,
                         want_column, accumulator);
}
RopeBuffer::Cursor cursor_at_point(RopeBuffer buffer, i64 want_line, i64 want_column)
{
  if (!buffer.rope.root.is_valid()) {
    RopeBuffer::Cursor cursor;
    cursor.current    = NodeRef::invalid();
    cursor.index      = 0;
    cursor.node_index = 0;
    cursor.summary    = {};
    return cursor;
  }

  want_line =
      std::min(std::max(want_line, 0ll), buffer.rope.get_summary_or_empty().newlines);
  return cursor_at_point(buffer, buffer.rope.root, want_line, want_column, {});
}

RopeBuffer::Cursor insert_position(RopeBuffer buffer, NodeRef root, i64 index,
                                   Summary accumulator)
{
  Node *root_val = buffer.rope.get(root);
  if (root_val->type == Node::Type::LEAF) {
    Summary summary = summarize(accumulator, summarize(root_val->data, index));

    RopeBuffer::Cursor cursor;
    cursor.current    = root;
    cursor.index      = summary.size;
    cursor.node_index = index;
    cursor.summary    = summary;
    return cursor;
  }

  Node *left    = buffer.rope.get(root_val->children.left);
  i64 left_size = left->summary.size;
  if (index <= left_size) {
    return insert_position(buffer, root_val->children.left, index, accumulator);
  }
  return insert_position(buffer, root_val->children.right, index - left_size,
                         summarize(accumulator, left->summary));
}
RopeBuffer::Cursor insert_position(RopeBuffer buffer, i64 index)
{
  if (!buffer.rope.root.is_valid()) {
    RopeBuffer::Cursor cursor;
    cursor.current    = NodeRef::invalid();
    cursor.index      = 0;
    cursor.node_index = 0;
    cursor.summary    = {};
    return cursor;
  }
  index = std::clamp(index, 0ll, buffer.rope.get_summary_or_empty().size);
  return insert_position(buffer, buffer.rope.root, index, {});
}

String buffer_to_string(RopeBuffer buffer, DynamicArray<u8> *builder)
{
  // TODO iterate through tree properly
  i64 size = buffer.rope.get_summary_or_empty().size;
  for (i64 i = 0; i < size; i++) {
    RopeBuffer::Cursor cursor = cursor_at(buffer, i);
    builder->push_back(char_at(buffer, cursor));
  }

  String string;
  string.data = builder->data;
  string.size = builder->size;
  return string;
}

void write_to_disk(RopeBuffer buffer)
{
  if (buffer.filename) {
    DynamicArray<u8> file_content_builder(&system_allocator);
    String contents = buffer_to_string(buffer, &file_content_builder);
    write_file(buffer.filename.value(), contents);
  }
}

// RopeBuffer::Cursor move_forward(RopeBuffer::Cursor c)
// {
//   Node *current = c.current;
//   i32 index     = c.node_index;

//   Summary summary = c.summary;
//   accumulate(&summary, char_at(c));

//   index++;
//   if (index >= current->data.size) {
//     Node *previous = current;
//     current        = previous->parent;
//     while (current && index_of_child(current, previous) == 1) {
//       previous = current;
//       current  = previous->parent;
//     }

//     if (!current) {
//       RopeBuffer::Cursor eof = c;
//       eof.node_index++;
//       eof.index++;
//       accumulate_eof(&c.summary);
//       return eof;
//     }

//     current = current->children[1];
//     while (current->type != Node::Type::LEAF) {
//       current = current->children[0];
//     }
//     index = 0;
//   }

//   RopeBuffer::Cursor ret;
//   ret.current    = current;
//   ret.node_index = index;
//   ret.index      = summary.size;
//   ret.summary    = summary;
//   return ret;
// }

// RopeBuffer::Iterator move_backward(RopeBuffer::Iterator c)
// {
//   i32 index     = c.node_index - 1;
//   Node *current = c.current;

//   if (index < 0) {
//     Node *previous = current;
//     current        = previous->parent;
//     while (current && index_of_child(current, previous) == 0) {
//       previous = current;
//       current  = previous->parent;
//     }

//     if (!current) {
//       return c;
//     }

//     current = current->children[0];
//     while (current->type != Node::Type::LEAF) {
//       current = current->children[1];
//     }
//     index = current->data.size - 1;
//   }

//   return RopeBuffer::Iterator{
//       current,
//       c.index - 1,
//       index,
//   };
// }

// RopeBuffer::Iterator move_backward_until(RopeBuffer::Iterator cursor, u8 c, i64 *count)
// {
//   (*count)                     = 0;
//   RopeBuffer::Iterator current = cursor;
//   RopeBuffer::Iterator next    = move_backward(current);
//   while (char_at(next) != c && is_valid(next) && next != current) {
//     current = next;
//     next    = move_backward(current);
//     (*count)++;
//   }
//   return current;
// }

// RopeBuffer::Cursor move_forward_until(RopeBuffer::Cursor cursor, u8 c, i64 *count)
// {
//   (*count)                   = 0;
//   RopeBuffer::Cursor current = cursor;
//   RopeBuffer::Cursor next    = move_forward(current);
//   while (char_at(current) != c && is_valid(next) && next != current) {
//     current = next;
//     next    = move_forward(current);
//     (*count)++;
//   }
//   return current;
// }

RopeBuffer::Cursor buffer_insert(RopeBuffer &buffer, RopeBuffer::Cursor cursor,
                                 u8 character)
{
  // if (!is_valid(cursor) && !is_eof(buffer, cursor)) {
  //   return cursor;
  // }

  // if (!buffer.editing_leaf.is_valid() || cursor != buffer.last_edit ||
  //     buffer.rope.get(buffer.editing_leaf)->data.size == CHUNK_MAX_SIZE) {
  NodeRef editing_leaf = insert_position(buffer, cursor.index).current;
  if (!editing_leaf.is_valid() || cursor != buffer.last_edit ||
      buffer.rope.get(editing_leaf)->data.size == CHUNK_MAX_SIZE ||
      buffer.rope.get(editing_leaf)->data.data == nullptr) {
    // String string;
    // string.data    = system_allocator.alloc(CHUNK_MAX_SIZE).data;
    // string.data[0] = character;
    // string.size    = 1;
    Chunk new_chunk;
    new_chunk.data    = system_allocator.alloc(CHUNK_MAX_SIZE).data;
    new_chunk.data[0] = character;
    new_chunk.size    = 1;

    Rope split_left;
    Rope split_right;
    split_left = split(buffer.rope, cursor.index, &split_right);
    release(buffer.rope);

    NodeRef editing_leaf = new_leaf(buffer.rope, new_chunk);
    Rope inserted        = insert_right(split_left, editing_leaf);
    release(split_left);

    Rope new_rope = concatanate(inserted, split_right);
    release(inserted);
    release(split_right);

    buffer.rope       = new_rope;
    cursor.current    = buffer.editing_leaf;
    cursor.node_index = 1;
  } else {
    Node *leaf_val           = buffer.rope.get(editing_leaf);
    Chunk *chunk             = &leaf_val->data;
    chunk->data[chunk->size] = character;
    chunk->size++;
    leaf_val->summary = summarize(leaf_val->data);
    restat_for_index(buffer.rope, cursor.index - (chunk->size - 1));
  }

  cursor           = cursor_at(buffer, cursor.index + 1);
  buffer.last_edit = cursor;
  return cursor;
}

RopeBuffer::Cursor buffer_remove(RopeBuffer &buffer, RopeBuffer::Cursor cursor)
{
  if (cursor.index <= 0) {
    return cursor;
  }

  Rope splits[4];
  splits[0] = split(buffer.rope, cursor.index - 1, &splits[1]);
  splits[2] = split(splits[1], 1, &splits[3]);

  Rope new_rope = merge(splits[0], splits[3]);
  if (!new_rope.root.is_valid()) {
    Chunk new_chunk;
    new_chunk.data = nullptr;
    new_chunk.size = 0;
    new_rope.root  = new_leaf(buffer.rope, new_chunk);
    new_rope       = commit_builder(new_rope);
  }

  release(buffer.rope);
  release(splits[0]);
  release(splits[1]);
  release(splits[2]);
  release(splits[3]);

  buffer.rope = new_rope;
  return cursor_at(buffer, cursor.index - 1);
}

bool is_valid(RopeBuffer buffer) { return buffer.rope.root.is_valid(); }

// tests

void rope_buffer_tests()
{
  File test_file;
  read_file("resources/test/big.txt", &system_allocator, &test_file);
  String text = test_file.data;

  // RopeBuffer buffer = create_rope_buffer();
  // buffer.rope       = rope_of("a");
  // for (i32 i = 0; i < text.size; i++) {
  //   buffer_insert(buffer, cursor_at(buffer, i), text[i]);
  // }

  // error(to_string(buffer.rope, &system_allocator));
}