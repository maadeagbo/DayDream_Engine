#pragma once

#include <experimental/filesystem>
#include <fstream>
#include <sstream>
#include "DD_Types.h"

enum class DD_IOflag : unsigned {
  READ = 0x1,
  WRITE = 0x2,
  APPEND = 0x4,
  DIRECTORY = 0x8
};
ENABLE_BITMASK_OPERATORS(DD_IOflag)

struct DD_IOhandle {
  ~DD_IOhandle();
  bool open(const char* fileName, const DD_IOflag flags);
  const char* readNextLine();
  void writeLine(const char* output);
  const char* getNextFile();

 private:
  typedef std::experimental::filesystem::directory_iterator fs_dir;
  char m_line[512];
  std::fstream m_file;
  fs_dir m_directory;
};
