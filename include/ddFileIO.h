#pragma once

#include "Container.h"
#include "StringLib.h"
#include <experimental/filesystem>
#include <fstream>

enum ddIOflag { READ = 0x1, WRITE = 0x2, APPEND = 0x4, DIRECTORY = 0x8 };

struct ddIO {
  ~ddIO();
  bool open(const char *fileName, const ddIOflag flags);
  const char *readNextLine();
  void writeLine(const char *output);
  inline dd_array<cbuff<512>> get_directory_files() { return dir_files; }

 private:
  char m_line[10000];
  std::fstream m_file;
  dd_array<cbuff<512>> dir_files;
};
