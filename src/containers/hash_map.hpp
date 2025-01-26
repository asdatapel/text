#pragma once

u32 hash(String str)
{
  u32 hash = 5381;
  for (int i = 0; i < str.size; i++) {
    int c = str.data[i];
    hash  = ((hash << 5) + hash) + c;
  }

  return hash;
}

u32 hash(void *ptr)
{
  return (u64)ptr;
}

template <typename KEY_TYPE, typename VALUE_TYPE>
struct HashMap {
  struct Element {
    KEY_TYPE key;
    VALUE_TYPE value;
    bool assigned = false;
  };

  Element *data = nullptr;
  i32 capacity  = 0;

  Allocator *allocator = nullptr;
  Mem allocation;

  HashMap(Allocator *allocator)
  {
    this->allocator = allocator;
    grow(1024);
  }

  VALUE_TYPE &operator[](KEY_TYPE key)
  {
    u32 starting_index = hash(key) % capacity;
    assert(data[starting_index].assigned && data[starting_index].key == key);
    return data[starting_index].value;
  }

  VALUE_TYPE *get(KEY_TYPE key)
  {
    u32 starting_index = hash(key) % capacity;
    if (data[starting_index].assigned && data[starting_index].key == key) {
      return &data[starting_index].value;
    }

    return nullptr;
  }

  VALUE_TYPE *put(KEY_TYPE key, VALUE_TYPE value)
  {
    u32 starting_index = hash(key) % capacity;
    assert(!data[starting_index].assigned);

    data[starting_index].assigned = true;
    data[starting_index].key      = key;
    data[starting_index].value    = value;

    return &data[starting_index].value;
  }

  void grow(i32 new_capacity)
  {
    if (!data) {
      this->allocation = allocator->alloc(new_capacity * sizeof(Element));
      this->data       = (Element *)allocation.data;
      this->capacity   = new_capacity;

      memset(this->data, 0, this->capacity * sizeof(Element));
      return;
    }

    Mem old_allocation = allocation;
    Element *old_data  = data;
    i32 old_capacity   = capacity;

    allocation     = allocator->alloc(new_capacity * sizeof(Element));
    this->data     = (Element *)allocation.data;
    this->capacity = new_capacity;
    memset(this->data, 0, this->capacity * sizeof(Element));
    for (i32 i = 0; i < old_capacity; i++) {
      if (old_data[i].assigned) {
        put(old_data[i].key, old_data[i].value);
      }
    }

    allocator->free(old_allocation);
  }
};