/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/
#pragma once

#include <cstdio>
#include <cstdlib>
#include <DD_Container.h>
#include <string.h>

static size_t getCharHash(const char* s)
{
	size_t h = 5381;
	int c;
	while ((c = *s++))
		h = ((h << 5) + h) + c;
	return h;
}

// small container (8 bytes + T)
template <const int T>
struct cbuff
{
	cbuff() { set(""); }
	cbuff(const char* in_str) { set(in_str); }
	int compare(const char* in_str)
	{
		return strcmp(cstr, in_str);
	}

	bool contains(const char* in_str)
	{
		return strstr(cstr, in_str) != nullptr;
	}

	bool operator==(const cbuff &other) const
	{
		return hash == other.hash;
	}

	cbuff& operator=(const char* in_str)
	{
		set(in_str);
		return *this;
	}

	bool operator<(const cbuff &other) const
	{
		return hash < other.hash;
	}

	void set(const char* in_str)
	{
		snprintf(cstr, T, "%s", in_str);
		hash = getCharHash(cstr);
	}

	template<typename... Args>
	void format(const char* format_str, const Args&... args)
	{
		snprintf(cstr, T, format_str, args...);
		hash = getCharHash(cstr);
	}

	const char* str() const { return cstr; }
	size_t gethash() const { return hash; }
private:
	char cstr[T];
	size_t hash;
};

namespace StrSpace
{
	/// \brief Take a string buffer and return a tokenized cbuff array
	/// WARNING: strToSplit will be cut off if greater than 512 chars
	template <const unsigned T>
	dd_array<cbuff<T>> tokenize512(const char* strToSplit, const char* delim)
	{
		char buff[512];
		snprintf(buff, 512, "%s", strToSplit);
		dd_array<cbuff<T>> output;

		// count number of delims
		const char* str_ptr = strToSplit;
		unsigned numTkns = 0, iter = 0;
		while (*str_ptr) {
			if (*str_ptr == *delim) { numTkns += 1; }
			str_ptr++;
		}
		numTkns += 1;
		output.resize(numTkns);
		// copy to array
		char* nxt = strtok(buff, delim);

		while (nxt && iter < output.size()) {
			output[iter].set(nxt);
			iter += 1;
			nxt = strtok(nullptr, delim);
		}
		return output;
	}
}
