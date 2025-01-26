#pragma once

#include "memory.hpp"
#include "types.hpp"

template <typename T>
struct Pool {
  struct Element {
    union {
      T value = {};
      i32 next;
    };
  };

  Element *data      = nullptr;
  i32 next_available = -1;
  i32 capacity       = 0;

  Pool() { init(32); }
  Pool(i32 capacity) { init(capacity); }

  void init(i32 capacity) { resize(capacity); }

  void resize(i32 new_capacity)
  {
    if (!data) {
      data = (Element *)sys_alloc(new_capacity * sizeof(Element));
    } else {
      sys_realloc(data, new_capacity * sizeof(Element));
    }

    i32 *next_available = &this->next_available;
    while (*next_available != -1) {
      next_available = &data[*next_available].next;
    }
    *next_available = capacity;

    for (i32 i = capacity; i < new_capacity - 1; i++) {
      data[i].next = i + 1;
    }
    data[new_capacity - 1].next = -1;

    capacity = new_capacity;
  }

  T &operator[](i32 i)
  {
    assert(i < capacity);
    return data[i].value;
  }

  T &wrapped_get(i32 i) { return data[i % capacity].value; }

  i32 push_back(T value)
  {
    if (next_available == -1) {
      resize(capacity * 2);
    };

    i32 idx          = next_available;
    Element *current = &data[idx];
    next_available   = current->next;
    current->value   = value;
    return idx;
  }

  void remove(i32 i)
  {
    Element *element = &data[i];
    element->next    = next_available;
    next_available   = i;
  }

  i32 index_of(T *ptr) { return ((Element *)ptr - data); }
};
