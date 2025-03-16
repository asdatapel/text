#pragma once

#include "containers/pool.hpp"
#include "string.hpp"

const i64 CHUNK_MAX_SIZE = 64;
struct Chunk {
  i64 index;
  i64 size;
};

struct Summary {
  i64 size            = 0;
  i64 newlines        = 0;
  i64 last_line_chars = 0;
};

struct Summarizer {
  DynamicArray<u8> *data;

  // void accumulate(Summary *summary, u8 c);
  // void accumulate_eof(Summary *summary);
  Summary summarize(const Summary &left, const Summary &right);
  Summary summarize(const Chunk &chunk);
  Summary summarize(const Chunk &right, i64 index);
};

////////////////////////////////////////////

struct NodeRef {
  i64 index;

  NodeRef() = default;
  NodeRef(i64 index) { this->index = index; }

  static NodeRef invalid() { return NodeRef(-1); }

  bool is_valid() { return index != -1; }

  bool operator!=(const NodeRef &other) { return index != other.index; }

  NodeRef for_builder()
  {
    i64 new_index = index ^ (1LL << 63);
    return NodeRef(new_index);
  }

  bool is_builder_ref() { return index & (1LL << 63); }
};

struct Node {
  enum struct Type {
    NODE,
    LEAF,
  };
  Type type;

  Summary summary;
  i32 ref_count = 0;
  // TODO: should be called height
  i64 depth = 0;

  union {
    struct {
      NodeRef left;
      NodeRef right;
    } children;

    Chunk data;
  };
};

struct Rope {
  Node empty = {
      .type    = Node::Type::LEAF,
      .depth   = 0,
      .summary = {},
      .data    = {0, 0},
  };
  NodeRef root;
  Pool<Node> *node_pool;
  DynamicArray<Node> *builder;
  Summarizer summarizer;

  Node *get_or_empty(NodeRef ref)
  {
    return ref.is_valid() ? &(*node_pool)[ref.index] : &empty;
  }
  const Summary &get_summary_or_empty() { return get_or_empty(root)->summary; };

  Node *get(NodeRef ref) { return &(*node_pool)[ref.index]; }
  Node *get_during_build(NodeRef ref)
  {
    if (ref.is_builder_ref()) {
      return &(*builder)[ref.for_builder().index];
    }

    return &(*node_pool)[ref.index];
  }
};

Rope create_rope(Summarizer summarizer)
{
  Rope rope;
  rope.root       = NodeRef::invalid();
  rope.node_pool  = new Pool<Node>();
  rope.builder    = new DynamicArray<Node>(&system_allocator);
  rope.summarizer = summarizer;
  return rope;
}

void fill_stats(Rope rope, Node *node)
{
  Node *left  = rope.get_during_build(node->children.left);
  Node *right = rope.get_during_build(node->children.right);

  node->summary = rope.summarizer.summarize(left->summary, right->summary);
  node->depth   = std::max(left->depth, right->depth) + 1;
}

void increment_ref_count(Rope rope, NodeRef ref)
{
  if (!ref.is_valid()) return;

  Node *node = rope.get(ref);
  node->ref_count++;
}
void release(Rope rope, NodeRef ref)
{
  if (!ref.is_valid()) return;

  Node *node = rope.get(ref);
  assert(node->ref_count > 0);

  node->ref_count--;
  if (node->ref_count == 0) {
    if (node->type == Node::Type::NODE) {
      release(rope, node->children.left);
      release(rope, node->children.right);
    }
    rope.node_pool->remove(ref.index);
  }
}
void release(Rope rope) { release(rope, rope.root); }
NodeRef commit_builder(Rope rope, NodeRef ref)
{
  if (!ref.is_valid()) return ref;

  if (ref.is_builder_ref()) {
    Node *node = rope.get_during_build(ref);
    if (node->type == Node::Type::NODE) {
      node->children.left  = commit_builder(rope, node->children.left);
      node->children.right = commit_builder(rope, node->children.right);
    }
    ref = rope.node_pool->push_back(*node);
  }
  increment_ref_count(rope, ref);
  return ref;
}
Rope commit_builder(Rope rope)
{
  rope.root = commit_builder(rope, rope.root);
  rope.builder->clear();
  return rope;
}

NodeRef new_node(Rope rope, NodeRef left, NodeRef right)
{
  Node node;
  node.type           = Node::Type::NODE;
  node.children.left  = left;
  node.children.right = right;
  fill_stats(rope, &node);

  i64 idx = rope.builder->push_back(node);
  return NodeRef(idx).for_builder();
}
NodeRef new_leaf(Rope rope, Chunk chunk)
{
  Node leaf;
  leaf.type    = Node::Type::LEAF;
  leaf.data    = chunk;
  leaf.summary = rope.summarizer.summarize(chunk);

  i64 idx = rope.builder->push_back(leaf);
  return NodeRef(idx).for_builder();
}
NodeRef copy_with_new_left(Rope rope, NodeRef ref, NodeRef left)
{
  Node *node = rope.get_during_build(ref);
  assert(node->type == Node::Type::NODE);
  return new_node(rope, left, node->children.right);
}
NodeRef copy_with_new_right(Rope rope, NodeRef ref, NodeRef right)
{
  Node *node = rope.get_during_build(ref);
  assert(node->type == Node::Type::NODE);
  return new_node(rope, node->children.left, right);
}

NodeRef rotate_left(Rope rope, NodeRef root)
{
  Node *root_val = rope.get_during_build(root);

  NodeRef right   = root_val->children.right;
  Node *right_val = rope.get_during_build(right);

  NodeRef new_left = copy_with_new_right(rope, root, right_val->children.left);
  NodeRef new_root = copy_with_new_left(rope, right, new_left);
  return new_root;
}

NodeRef rotate_right(Rope rope, NodeRef root)
{
  Node *root_val = rope.get_during_build(root);

  NodeRef left   = root_val->children.left;
  Node *left_val = rope.get_during_build(left);

  NodeRef new_right = copy_with_new_left(rope, root, left_val->children.right);
  NodeRef new_root  = copy_with_new_right(rope, left, new_right);
  return new_root;
}

i32 get_balance(Rope rope, NodeRef left, NodeRef right)
{
  Node *left_val  = rope.get_during_build(left);
  Node *right_val = rope.get_during_build(right);
  return left_val->depth - right_val->depth;
}

NodeRef balance(Rope rope, NodeRef root)
{
  Node *root_val = rope.get_during_build(root);
  if (root_val->type == Node::Type::LEAF) {
    return root;
  }

  Node *left  = rope.get_during_build(root_val->children.left);
  Node *right = rope.get_during_build(root_val->children.right);
  i64 balance = right->depth - left->depth;
  if (balance > 1) {  // right heavy
    // right should be a Node
    Node *sub_left  = rope.get_during_build(right->children.left);
    Node *sub_right = rope.get_during_build(right->children.right);
    if (sub_left->depth > sub_right->depth) {  // right-left
      root_val->children.right = rotate_right(rope, root_val->children.right);
    }
    return rotate_left(rope, root);
  }
  if (balance < -1) {  // left heavy
    // left should be a Node
    Node *sub_left  = rope.get_during_build(left->children.left);
    Node *sub_right = rope.get_during_build(left->children.right);
    if (sub_right->depth > sub_left->depth) {  // left-right
      root_val->children.left = rotate_left(rope, root_val->children.left);
    }
    return rotate_right(rope, root);
  }

  return root;
}

// TODO: combine balance and insert
NodeRef insert(Rope rope, NodeRef root, NodeRef node, i64 index)
{
  if (!root.is_valid()) {
    return node;
  }

  Node *root_val = rope.get_during_build(root);
  if (root_val->type == Node::Type::LEAF) {
    if (index <= 0) {
      return new_node(rope, node, root);
    } else {
      return new_node(rope, root, node);
    }
  }

  i64 left_size = rope.get_during_build(root_val->children.left)->summary.size;
  if (index < left_size) {
    NodeRef left_with_inserted = insert(rope, root_val->children.left, node, index);
    return balance(rope, copy_with_new_left(rope, root, left_with_inserted));
  } else {
    NodeRef right_with_inserted =
        insert(rope, root_val->children.right, node, index - left_size);
    return balance(rope, copy_with_new_right(rope, root, right_with_inserted));
  }
}
Rope insert(Rope rope, NodeRef node, i64 index)
{
  NodeRef new_root = insert(rope, rope.root, node, index);

  Rope new_rope = rope;
  new_rope.root = new_root;
  new_rope      = commit_builder(new_rope);
  return new_rope;
}

NodeRef insert_right(Rope rope, NodeRef root, NodeRef node)
{
  if (!root.is_valid()) {
    return node;
  }

  Node *root_val = rope.get_during_build(root);
  if (root_val->type == Node::Type::LEAF) {
    return new_node(rope, root, node);
  }

  NodeRef right_with_inserted = insert_right(rope, root_val->children.right, node);
  return balance(rope, copy_with_new_right(rope, root, right_with_inserted));
}
Rope insert_right(Rope rope, NodeRef node)
{
  NodeRef new_root = insert_right(rope, rope.root, node);

  Rope new_rope = rope;
  new_rope.root = new_root;
  new_rope      = commit_builder(new_rope);
  return new_rope;
}

// Rope rope_of(String text)
// {
//   Rope rope;
//   rope.node_pool = new Pool<Node>(32);
//   rope.builder   = new DynamicArray<Node>(&system_allocator);
//   rope.root      = NodeRef::invalid();
//   rope.summarizer =

//   i64 next_position = 0;
//   while (next_position < text.size) {
//     Chunk chunk;
//     chunk.index  = next_position;
//     chunk.size   = std::min(text.size - next_position, CHUNK_MAX_SIZE);
//     NodeRef leaf = new_leaf(rope, chunk);

//     if (!rope.root.is_valid()) {
//       rope.root = leaf;
//       rope      = commit_builder(rope);
//     } else {
//       Rope new_rope = insert_right(rope, leaf);
//       release(rope);
//       rope = new_rope;
//     }

//     next_position += chunk.size;
//   }

//   return rope;
// };

// TODO this creates empty nodes if the index is at the beginning or end of a chunk
NodeRef split(Rope rope, NodeRef root, i64 index, NodeRef *right_ret)
{
  NodeRef new_left  = NodeRef::invalid();
  NodeRef new_right = NodeRef::invalid();

  Node *root_val = rope.get_during_build(root);

  if (root_val->type == Node::Type::LEAF) {
    Chunk chunk = root_val->data;
    Chunk left_chunk;
    Chunk right_chunk;
    left_chunk.index  = chunk.index;
    left_chunk.size   = index;
    right_chunk.index = chunk.index + index;
    right_chunk.size  = chunk.size - index;

    if (left_chunk.size > 0) {
      new_left = new_leaf(rope, left_chunk);
    }
    if (right_chunk.size > 0) {
      new_right = new_leaf(rope, right_chunk);
    }
  } else {
    Node *left_val = rope.get_during_build(root_val->children.left);
    if (index < left_val->summary.size) {
      NodeRef split_child_left;
      NodeRef split_child_right;
      split_child_left = split(rope, root_val->children.left, index, &split_child_right);

      new_left = split_child_left;
      if (split_child_right.is_valid()) {
        new_right = balance(rope, copy_with_new_left(rope, root, split_child_right));
      } else {
        new_right = root_val->children.right;
      }
    } else {
      NodeRef split_child_left;
      NodeRef split_child_right;
      split_child_left = split(rope, root_val->children.right,
                               index - left_val->summary.size, &split_child_right);

      new_right = split_child_right;
      if (split_child_left.is_valid()) {
        new_left = balance(rope, copy_with_new_right(rope, root, split_child_left));
      } else {
        new_left = root_val->children.left;
      }
    }
  }

  (*right_ret) = new_right;
  return new_left;
}
Rope split(Rope rope, i64 index, Rope *right_ret)
{
  NodeRef new_left_root  = NodeRef::invalid();
  NodeRef new_right_root = NodeRef::invalid();
  if (rope.root.is_valid()) {
    new_left_root = split(rope, rope.root, index, &new_right_root);
  }

  Rope new_left_rope = rope;
  new_left_rope.root = new_left_root;
  new_left_rope.root = commit_builder(new_left_rope, new_left_root);

  Rope new_right_rope = rope;
  new_right_rope.root = new_right_root;
  new_right_rope.root = commit_builder(new_right_rope, new_right_root);

  rope.builder->clear();

  (*right_ret) = new_right_rope;
  return new_left_rope;
}

NodeRef merge(Rope rope, NodeRef left, NodeRef right)
{
  if (!left.is_valid()) return right;
  if (!right.is_valid()) return left;

  Node *right_val = rope.get_during_build(right);
  if (right_val->type == Node::Type::LEAF) {
    NodeRef new_ref = insert_right(rope, left, right);
    return new_ref;
  }

  NodeRef merged_left  = merge(rope, left, right_val->children.left);
  NodeRef merged_right = merge(rope, merged_left, right_val->children.right);
  return merged_right;
}
Rope merge(Rope left, Rope right)
{
  NodeRef new_root = merge(left, left.root, right.root);

  Rope new_rope = left;
  new_rope.root = new_root;
  new_rope      = commit_builder(new_rope);
  return new_rope;
}

NodeRef concatanate_left(Rope rope, NodeRef left, NodeRef right)
{
  if (get_balance(rope, left, right) >= -1) {
    return new_node(rope, left, right);
  }

  Node *right_val     = rope.get_during_build(right);
  NodeRef merged_left = concatanate_left(rope, left, right_val->children.left);
  return balance(rope, copy_with_new_left(rope, right, merged_left));
}
NodeRef concatanate_right(Rope rope, NodeRef left, NodeRef right)
{
  if (get_balance(rope, left, right) <= 1) {
    return new_node(rope, left, right);
  }

  Node *left_val       = rope.get_during_build(left);
  NodeRef merged_right = concatanate_right(rope, left_val->children.right, right);
  return balance(rope, copy_with_new_right(rope, left, merged_right));
}
Rope concatanate(Rope left, Rope right)
{
  if (!left.root.is_valid()) {
    return commit_builder(right);
  }
  if (!right.root.is_valid()) {
    return commit_builder(left);
  }

  NodeRef new_root;
  i32 balance = get_balance(left, left.root, right.root);
  if (balance < 0) {
    new_root = concatanate_left(left, left.root, right.root);
  } else {
    new_root = concatanate_right(left, left.root, right.root);
  }

  Rope new_rope = left;
  new_rope.root = new_root;
  new_rope      = commit_builder(new_rope);
  return new_rope;
}

// THIS IS A MUTATE
void restat_for_index(Rope rope, NodeRef root, i64 index)
{
  Node *root_val = rope.get(root);

  if (root_val->type == Node::Type::LEAF) {
    root_val->summary = rope.summarizer.summarize(root_val->data);
    return;
  }

  i64 left_size = rope.get(root_val->children.left)->summary.size;
  if (index < left_size) {
    restat_for_index(rope, root_val->children.left, index);
  } else {
    restat_for_index(rope, root_val->children.right, index - left_size);
  }
  fill_stats(rope, root_val);
}
void restat_for_index(Rope rope, i64 index) { restat_for_index(rope, rope.root, index); }

/////////////////////////////

void test_rope() {}