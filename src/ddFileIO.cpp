#include "ddFileIO.h"
#include <string>
#include <vector>
namespace fs = std::experimental::filesystem;
typedef std::experimental::filesystem::directory_iterator fs_dir;

/** \brief Parse directory and store in dd_array */
dd_array<cbuff<512>> parse_directory(fs_dir &dir_handle);

ddIO::~ddIO() {
  if (m_file.is_open()) {
    m_file.close();
  }
}

/// \brief Open a file or directory (based on ddIOflag)
bool ddIO::open(const char *fileName, const ddIOflag flags) {
  std::ios_base::openmode ios_flag = std::ios::in;
  if ((unsigned)(flags & ddIOflag::READ)) {
    ios_flag = std::ios::in;
  } else if ((unsigned)(flags & ddIOflag::WRITE)) {
    ios_flag = std::ios::out;
  } else if ((unsigned)(flags & ddIOflag::APPEND)) {
    ios_flag = std::ios::app;
  } else if ((unsigned)(flags & ddIOflag::DIRECTORY)) {
    fs_dir directory = fs_dir(fileName);
    if (directory != fs::end(directory)) {
      dir_files = std::move(parse_directory(directory));
      return true;
    }
    return false;
  }

  m_file.open(fileName, ios_flag);
  return m_file.good();
}

/// \brief Read the next line in a file
const char *ddIO::readNextLine() {
  if (!m_file.eof()) {
    m_file.getline(m_line, 10000);
    return (const char *)m_line;
  }
  return nullptr;
}

/// \brief Write a line to an already opened file
void ddIO::writeLine(const char *output) {
  if (m_file.good()) {
    m_file << output;
  }
}

dd_array<cbuff<512>> parse_directory(fs_dir &dir_handle) {
  dd_array<cbuff<512>> files;
  cbuff<512> fname;
  std::vector<std::string> vec_files;
  // count the number of files
  unsigned num_files = 0;
  for (auto &p : fs::directory_iterator(dir_handle)) {
    fname.format("%s", p.path().string().c_str());
    vec_files.push_back(p.path().string());
    num_files++;
  }

  files.resize(num_files);
  // need to sort vector since directory is not sorted on all filesystems
  std::sort(vec_files.begin(), vec_files.end());

  unsigned file_idx = 0;
  for (auto &p : vec_files) {
    fname.format("%s", p.c_str());
    files[file_idx] = fname;
    file_idx++;
  }

  return files;
}