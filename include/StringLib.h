/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/
#pragma once

#include "Container.h"

/** \brief A small string manipulation class for char[T], small container (8 bytes + T)*/
template <const int T>
struct cbuff {
  cbuff();
  
  cbuff(const char *in_string);
  
  bool compare(const char *in_string) const;

  bool contains(const char *in_string) const;

	bool is_empty() const;

  bool operator==(const cbuff &other) const;

  cbuff &operator=(const char *in_string);

	template <const int U, const int V = ((U > T) ? U : T) >
	cbuff<V> operator+(const cbuff<U> &other) const;

	template <const int U>
	cbuff &operator+=(const cbuff<U> &other);

	cbuff operator+(const char* in_string) const;

	cbuff &operator+=(const char* in_string);

  template <const int U> cbuff<T>(const cbuff<U> &other);

  template <const int U> cbuff<T> &operator=(const cbuff<U> &in_cbuff);

  bool operator<(const cbuff &other) const;

  void set(const char *in_string);

  template <typename... Args>
  void format(const char *format_str, const Args &... args)
	{
		char bin[T];
		snprintf(bin, T, format_str, args...);
		set(bin);
	}

  int find(const char *in_string) const;

  void replace(const char search_token, const char replace_token);

  const char *str(const unsigned index = 0) const;

  cbuff trim(const unsigned index, const unsigned length = 0) const;

  size_t gethash() const;

 private:
  char char_bin[T];
  size_t hash;
};

typedef cbuff<8> string8;
typedef cbuff<16> string16;
typedef cbuff<32> string32;
typedef cbuff<64> string64;
typedef cbuff<128> string128;
typedef cbuff<256> string256;
typedef cbuff<512> string512;
typedef cbuff<1024> string1024;

namespace StrLib
{
  size_t get_char_hash(const char *input);

  dd_array<unsigned> tokenize(const char *input, const char *delimeters);

  template <const unsigned T>
  dd_array<cbuff<T>> tokenize2(const char *input, const char *delimeters);
} // StrLib

