#pragma once

#include <experimental/filesystem>
#include <fstream>
#include <sstream>

template <typename Enum>
struct EnableBitMaskOperators {
  static const bool enable = false;
};

#define ENABLE_BITMASK_OPERATORS(TYPE)  \
  template <>                           \
  struct EnableBitMaskOperators<TYPE> { \
    static const bool enable = true;    \
  };

// bitwise Or
template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator|(Enum lhs, Enum rhs) {
  using underlying = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(static_cast<underlying>(lhs) |
                           static_cast<underlying>(rhs));
}

// bitwise Or-Eq
template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator|=(Enum &lhs, Enum rhs) {
  using underlying = typename std::underlying_type<Enum>::type;
  lhs = static_cast<Enum>(static_cast<underlying>(lhs) |
                          static_cast<underlying>(rhs));
  return lhs;
}

// bitwise And
template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator&(Enum lhs, Enum rhs) {
  using underlying = typename std::underlying_type<Enum>::type;
  return static_cast<Enum>(static_cast<underlying>(lhs) &
                           static_cast<underlying>(rhs));
}

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
