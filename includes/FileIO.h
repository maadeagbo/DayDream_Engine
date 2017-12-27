#pragma once

#include <experimental/filesystem>
#include <fstream>
#include <sstream>
#include "Types.h"

enum class ddIOflag : unsigned {
  READ = 0x1,
  WRITE = 0x2,
  APPEND = 0x4,
  DIRECTORY = 0x8
};
ENABLE_BITMASK_OPERATORS(ddIOflag)

struct ddIO {
  ~ddIO();
  bool open(const char* fileName, const ddIOflag flags);
  const char* readNextLine();
  void writeLine(const char* output);
  const char* getNextFile();

 private:
  typedef std::experimental::filesystem::directory_iterator fs_dir;
  char m_line[512];
  std::fstream m_file;
  fs_dir m_directory;
};
