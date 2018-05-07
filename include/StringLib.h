/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/
#pragma once

#include "Container.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>

/** \brief Hashes const char* strings */
static size_t getCharHash(const char *s) {
  size_t h = 5381;
  int c;
  while ((c = *s++)) h = ((h << 5) + h) + c;
  return h;
}

// small container (8 bytes + T)
/** \brief A small string manipulation class for char[T] */
template <const int T>
struct cbuff {
  cbuff() { set(""); }
  cbuff(const char *in_str) { set(in_str); }
  /** \brief Performs strcmp(cstr, in_str) (0 means equal) */
  int compare(const char *in_str) { return strcmp(cstr, in_str); }

  /** \brief Returns true if cstr contains in_str */
  bool contains(const char *in_str) { return strstr(cstr, in_str) != nullptr; }

  /** \brief Compares the hashed value of the two cbuff's (fast) */
  bool operator==(const cbuff &other) const { return hash == other.hash; }

  /** \brief Sets and hashes internal cstr to in_str */
  cbuff &operator=(const char *in_str) {
    set(in_str);
    return *this;
  }

  /** \brief Performs fast less-than coparison (this->hash < other->hash) */
  bool operator<(const cbuff &other) const { return hash < other.hash; }

  /** \brief Sets and hashes internal cstr to in_str */
  void set(const char *in_str) {
    snprintf(cstr, T, "%s", in_str);
    hash = getCharHash(cstr);
  }

  /** \brief Sets and hashes internal cstr using format string */
  template <typename... Args>
  void format(const char *format_str, const Args &... args) {
    snprintf(cstr, T, format_str, args...);
    hash = getCharHash(cstr);
  }

  /** \brief Returns const char* internal representation */
  const char *str() const { return cstr; }
  /** \brief Returns hashed string internal representation */
  size_t gethash() const { return hash; }

 private:
  char cstr[T];
  size_t hash;
};

/** \brief Simple operations on cbuff containers */
namespace StrSpace {
/**
 * \brief Take a string buffer and return a tokenized cbuff array
 * WARNING: strToSplit will be cut off if greater than 1024 chars
 */
template <const unsigned T>
dd_array<cbuff<T>> tokenize1024(const char *strToSplit, const char *delim) {
  char buff[1024];
  snprintf(buff, 1024, "%s", strToSplit);
  dd_array<cbuff<T>> output;

  // count number of delims
  const char *str_ptr = strToSplit;
  unsigned numTkns = 0, iter = 0;
  while (*str_ptr) {
    if (*str_ptr == *delim) {
      numTkns += 1;
    }
    str_ptr++;
  }
  numTkns += 1;
  output.resize(numTkns);
  // copy to array
  char *nxt = strtok(buff, delim);

  while (nxt && iter < output.size()) {
    output[iter].set(nxt);
    iter += 1;
    nxt = strtok(nullptr, delim);
  }
  return output;
}
}
