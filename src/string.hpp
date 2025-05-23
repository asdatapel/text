#pragma once

#include <cassert>
#include <cstring>

#include "memory.hpp"
#include "types.hpp"

struct String {
  u8 *data = nullptr;
  i64 size = 0;

  String() {}

  String(u8 *data, i64 size)
  {
    this->data = data;
    this->size = size;
  }

  template <i64 N>
  String(const char (&str)[N])
  {
    data = (u8 *)&str;
    size = N - 1;  // remove '\0'
  }

  u8 &operator[](i64 i)
  {
    assert(i < size);
    return data[i];
  }

  String sub(i64 from, i64 to)
  {
    assert(from <= size);
    assert(to <= size);

    return String(data + from, to - from);
  }

  b8 operator==(const String &other)
  {
    if (size != other.size) return false;
    for (i32 i = 0; i < size; i++) {
      if (data[i] != other.data[i]) return false;
    }
    return true;
  }

  bool starts_with(const String& prefix) {
    if (prefix.size > size) return false;
  
    for (i32 i = 0; i < prefix.size; i++) {
      if (data[i] != prefix.data[i]) return false;
    }
    return true;
  }

  u64 to_u64()
  {
    u64 val = 0;
    for (int i = 0; i < size; i++) {
      if (data[i] < '0' && data[i] > '9') return val;
      val = 10 * val + (data[i] - '0');
    }
    return val;
  }

  char *c_str(Allocator *allocator)
  {
    char *null_terminated = (char *)allocator->alloc(size + 1).data;
    memcpy(null_terminated, data, size);
    null_terminated[size] = '\0';
    return null_terminated;
  }

  String copy(Allocator *allocator)
  {
    String copy;
    copy.size = size;
    copy.data = (u8 *)allocator->alloc(size).data;
    memcpy(copy.data, data, size);
    return copy;
  }
};

struct NullTerminatedString : String {
  static NullTerminatedString concatenate(String str1, String str2, Allocator *allocator)
  {
    NullTerminatedString new_string;
    new_string.size = str1.size + str2.size + 1;
    new_string.data = (u8 *)allocator->alloc(new_string.size).data;

    memcpy(new_string.data, str1.data, str1.size);
    memcpy(new_string.data + str1.size, str2.data, str2.size);
    new_string.data[new_string.size - 1] = '\0';

    return new_string;
  }
};

template <i64 CAPACITY>
struct StaticString {
  inline static const i64 MAX_SIZE = CAPACITY;
  u8 data[CAPACITY + 1];
  i64 size = 0;

  StaticString() { data[0] = '\0'; }

  template <i64 CAPACITY_2>
  StaticString(StaticString<CAPACITY_2> &str2)
  {
    size = fmin(str2.size, MAX_SIZE);
    memcpy(data, str2.data, size);
    data[size] = '\0';
  }

  template <i64 N>
  StaticString(const char (&str)[N])
  {
    assert(N <= CAPACITY + 1);
    memcpy(data, str, N);
    size = N - 1;
  }

  StaticString(const String &str2)
  {
    assert(str2.size <= CAPACITY + 1);
    memcpy(data, str2.data, CAPACITY);
    size       = str2.size;
    data[size] = '\0';
  }

  template <i64 CAPACITY_2>
  void operator=(const StaticString<CAPACITY_2> &str2)
  {
    size = fmin(str2.size, MAX_SIZE);
    memcpy(data, str2.data, size);
    data[size] = '\0';
  }

  void operator=(const String &str2)
  {
    size = fmin(str2.size, MAX_SIZE);
    memcpy(data, str2.data, size);
    data[size] = '\0';
  }

  String to_str() { return String(data, size); }
  operator String() { return to_str(); }

  u8 &operator[](i64 i)
  {
    assert(i < size);
    return data[i];
  }

  void shift_delete_range(i64 from, i64 to)
  {
    assert(from >= 0);
    assert(from < size);
    assert(to >= 0);
    assert(to <= size);

    size -= to - from;

    while (from < size) {
      data[from] = data[to];
      from++;
      to++;
    }
  }

  void shift_delete(i64 i)
  {
    assert(i >= 0);
    assert(i < size);

    shift_delete_range(i, i + 1);
  }

  void push_middle(u8 value, i64 i)
  {
    assert(i >= 0);
    assert(i <= size);

    i64 j = size;
    while (j > i) {
      data[j] = data[j - 1];
      j--;
    }
    data[i] = value;
    size++;
  }

  static StaticString<CAPACITY> from_i32(i32 x)
  {
    StaticString<CAPACITY> str;
    str.size = snprintf((char *)str.data, CAPACITY, "%i", x);
    return str;
  }
};

bool is_only_whitespace(String text)
{
  for (i64 i = 0; i < text.size; i++) {
    if (!std::isspace(text[i])) {
      return false;
    }
  }
  return true;
}
