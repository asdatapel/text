#pragma once

#include "memory.hpp"
#include "types.hpp"

template <typename T>
struct Pool {
  struct Element {
    union {
      T value = {};
      i64 next;
    };
  };

  Element *data      = nullptr;
  i64 next_available = -1;
  i64 capacity       = 0;

  Pool() { init(32); }
  Pool(i64 capacity) { init(capacity); }

  void init(i64 capacity) { resize(capacity); }

  void resize(i64 new_capacity)
  {
    if (!data) {
      data = (Element *)sys_alloc(new_capacity * sizeof(Element));
    } else {
      data = (Element *)sys_realloc(data, new_capacity * sizeof(Element));
    }

    i64 *next_available = &this->next_available;
    while (*next_available != -1) {
      next_available = &data[*next_available].next;
    }
    *next_available = capacity;

    for (i64 i = capacity; i < new_capacity - 1; i++) {
      data[i].next = i + 1;
    }
    data[new_capacity - 1].next = -1;

    capacity = new_capacity;
  }

  T &operator[](i64 i)
  {
    assert(i > -1);
    assert(i < capacity);
    return data[i].value;
  }
  T &wrapped_get(i64 i) { return data[i % capacity].value; }

  i64 push_back(T value)
  {
    if (next_available == -1) {
      resize(capacity * 2);
    };

    i64 idx          = next_available;
    Element *current = &data[idx];
    next_available   = current->next;
    current->value   = value;
    return idx;
  }

  void remove(i64 i)
  {
    Element *element = &data[i];
    element->next    = next_available;
    next_available   = i;
  }

  i64 index_of(T *ptr) { return ((Element *)ptr - data); }

  i64 count_free()
  {
    i64 count = 0;
    i64 next  = next_available;
    while (next != -1) {
      count++;
      next = data[next].next;
    }
    return count;
  }
};
