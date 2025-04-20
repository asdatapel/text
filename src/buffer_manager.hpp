#pragma once

#include "rope_buffer.hpp"
#include "containers/hash_map.hpp"
#include "containers/pool.hpp"

struct BufferManager {
  Pool<RopeBuffer> buffers;
  HashMap<String, i32> named_buffers;

  BufferManager() : buffers(1024), named_buffers(HashMap<String, i32>(&system_allocator))
  {
  }

  RopeBuffer *get_or_open_buffer(String filename)
  {
    i32 *existing_buffer_idx = named_buffers.get(filename);
    if (existing_buffer_idx) {
      info("Using existing buffer: ", filename);
      return &buffers[*existing_buffer_idx];
    }

    info("Opening buffer: ", filename);
    RopeBuffer new_buffer = load_rope_buffer(filename);
    i32 new_buffer_idx    = buffers.push_back(new_buffer);
    named_buffers.put(filename, new_buffer_idx);
    return &buffers[new_buffer_idx];
  }

  RopeBuffer *create_buffer()
  {
    RopeBuffer new_buffer = ::create_rope_buffer();
    return &buffers[buffers.push_back(new_buffer)];
  }
};
BufferManager buffer_manager;