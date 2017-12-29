#include "ddFileIO.h"
namespace fs = std::experimental::filesystem;

ddIO::~ddIO() {
  if (m_file.is_open()) {
    m_file.close();
  }
}

/// \brief Open a file or directory (based on ddIOflag)
bool ddIO::open(const char* fileName, const ddIOflag flags) {
  std::ios_base::openmode ios_flag = std::ios::in;
  if ((unsigned)(flags & ddIOflag::READ)) {
    ios_flag = std::ios::in;
  } else if ((unsigned)(flags & ddIOflag::WRITE)) {
    ios_flag = std::ios::out;
    if ((unsigned)(flags & ddIOflag::APPEND)) {
      ios_flag |= std::ios::app;
    }
  } else if ((unsigned)(flags & ddIOflag::DIRECTORY)) {
    m_directory = fs_dir(fileName);
    return m_directory != fs::end(m_directory);
  }

  m_file.open(fileName, ios_flag);
  return m_file.good();
}

/// \brief Read the next line in a file
const char* ddIO::readNextLine() {
  if (m_file.getline(m_line, 512)) {
    return (const char*)m_line;
  }
  return nullptr;
}

/// \brief Write a line to an already opened file
void ddIO::writeLine(const char* output) {
  if (m_file.good()) {
    m_file << output;
  }
}

/// \brief Read the next file in accessed directory
const char* ddIO::getNextFile() {
  if (m_directory == fs::end(m_directory)) {
    return " ";
  }
  snprintf(m_line, 512, "%s", (*m_directory).path().string().c_str());
  m_directory++;
  return (const char*)m_line;
}
