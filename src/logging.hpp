#pragma once

#include <stdio.h>
#include <iostream>

#include "string.hpp"
#include "containers/array.hpp"

struct StringBuilder {
  String string;
  Mem mem;

  StringBuilder()
  {
    mem = system_allocator.alloc(1024);
    string.data = mem.data;
    string.size = 0;
  }

  i32 append(String in)
  {
    if (in.size > mem.size - string.size) {
      system_allocator.resize(mem, mem.size * 2);
    }

    i32 index = string.size;
    memcpy(string.data + string.size, in.data, in.size);
    string.size += in.size;

    return index;
  }
};
struct Logs {
  struct Entry {
    i32 string_index;
    i32 string_size;

    i32 severity;
  };

  StringBuilder string_builder;
  Array<Entry, 4096> entries;

  void log(String value)
  {
    Entry entry;
    entry.string_index = string_builder.append(value);
    entry.string_size = value.size;

    entries.push_back(entry);
  }
};

Logs logs;



std::ostream& operator<<(std::ostream& out, String str)
{
  out.write((char*)str.data, str.size);

  return out;
}

template <class... Args>
void fatal(Args... args)
{
  (std::cout << ... << args) << std::endl;
  abort();
}

template <class... Args>
void error(Args... args)
{
  std::cout << "Error: ";
  (std::cout << ... << args) << std::endl;
}

template <class... Args>
void warning(Args... args)
{
  (std::cout << ... << args) << std::endl;
}

template <class... Args>
void info(Args... args)
{
  (std::cout << ... << args) << std::endl;
}
