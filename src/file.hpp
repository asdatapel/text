#pragma once

#include <fstream>
#include <iostream>

#include "memory.hpp"
#include "string.hpp"

struct File {
  String path;
  String data;
  Mem mem;
};

bool read_file(String path, Allocator *allocator, File *file)
{
  Temp tmp;
  std::ifstream in_stream(path.c_str(&tmp), std::ios::binary | std::ios::ate);
  if (!in_stream.is_open()) {
    return false;
  }

  u64 file_size = in_stream.tellg();
  file->mem     = allocator->alloc(path.size + file_size);

  file->path.data = file->mem.data;
  file->path.size = path.size;
  memcpy(file->path.data, path.data, path.size);

  file->data.data = file->mem.data + file->path.size;
  file->data.size = file_size;

  in_stream.seekg(0, std::ios::beg);
  in_stream.read((char *)file->data.data, file->data.size);
  in_stream.close();

  return true;
}

void write_file(String filename, String file, b8 overwrite = false)
{
  std::ios_base::openmode file_flags = std::ios::out | std::ios::binary;
  if (overwrite)
    file_flags |= std::ios::trunc;
  else
    file_flags |= std::ios::ate;

  std::fstream out_stream(filename.c_str(&tmp_allocator), file_flags);
  out_stream.write((char *)file.data, file.size);
  out_stream.close();
}
