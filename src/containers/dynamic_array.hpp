#pragma once

#include <string>

#include "memory.hpp"

template <typename T>
struct DynamicArray {
  T *data = nullptr;
  i32 size = 0;
  i32 capacity = 0;

  Allocator *allocator = nullptr;
  Mem allocation;

  DynamicArray(Allocator *allocator)
  {
    this->allocator = allocator;

    set_capacity(8);
  }

  T &operator[](i32 i)
  {
    assert(i < size);
    return data[i];
  }

  T &operator[](u32 i)
  {
    assert(i < size);
    return data[i];
  }

  i32 push_back(T val)
  {
    if (capacity < size + 1) {
      set_capacity(capacity * 2);
    }

    data[size] = val;
    return size++;
  }

  i32 insert(i32 i, T val)
  {
    if (capacity < size + 1) {
      set_capacity(capacity * 2);
    }

    memcpy(&data[i + 1], &data[i], sizeof(T) * (size - i));
    data[i] = val;
    size++;
    return i;
  }

  void swap_delete(i32 i)
  {
    data[i] = data[size - 1];
    size--;
  }

  void shift_delete(i32 i)
  {
    while (i + 1 < size) {
      data[i] = data[i + 1];
      i++;
    }
    size--;
  }

  void resize(i32 new_size)
  {
    if (capacity < new_size) {
        i32 new_capacity = capacity;
        while (new_capacity < new_size) new_capacity *= 2;
        set_capacity(new_capacity);
    }
    size = new_size;
  }

  void clear() { size = 0; }

  i32 index_of(T *elem)
  {
    i32 index = elem - data;
    if (index >= 0 && index < size) return index;
    return -1;
  }

  void set_capacity(i32 capacity)
  {
    assert(capacity >= size);
  
    if (!data) {
        allocation = allocator->alloc(capacity * sizeof(T));
        data = (T*)allocation.data;
        this->capacity = capacity;
        return;
    }
    

    Mem old_allocation = allocation;
    allocation = allocator->alloc(capacity * sizeof(T));
    memcpy(allocation.data, old_allocation.data, old_allocation.size);
    allocator->free(old_allocation);

    data = (T*)allocation.data;
    this->capacity = capacity;
  }
};