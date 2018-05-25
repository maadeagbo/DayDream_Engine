#include "StringLib.h"
#include <cstdlib>
#include <string.h>

#define EXPLICIT_INST(SIZE)             \
  template cbuff<SIZE>::cbuff(const cbuff<8> &other); \
  template cbuff<SIZE>::cbuff(const cbuff<16> &other); \
  template cbuff<SIZE>::cbuff(const cbuff<32> &other); \
  template cbuff<SIZE>::cbuff(const cbuff<64> &other); \
  template cbuff<SIZE>::cbuff(const cbuff<128> &other); \
  template cbuff<SIZE>::cbuff(const cbuff<256> &other); \
  template cbuff<SIZE>::cbuff(const cbuff<512> &other); \
  template cbuff<SIZE>::cbuff(const cbuff<1024> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<8> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<16> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<32> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<64> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<128> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<256> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<512> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator+=(const cbuff<1024> &other); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<8> &in_cbuff); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<16> &in_cbuff); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<32> &in_cbuff); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<64> &in_cbuff); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<128> &in_cbuff); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<256> &in_cbuff); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<512> &in_cbuff); \
  template cbuff<SIZE>& cbuff<SIZE>::operator=(const cbuff<1024> &in_cbuff);

template <const int T>
cbuff<T>::cbuff()
{
  set("");
}

template <const int T>
cbuff<T>::cbuff(const char *in_string) 
{ 
  set(in_string);
}

template <const int T>
bool cbuff<T>::compare(const char *in_string) const
{ 
  return strcmp(char_bin, in_string) == 0; 
}

template <const int T>
bool cbuff<T>::contains(const char *in_string) const
{ 
  return strstr(char_bin, in_string) != nullptr; 
}

template<int T>
bool cbuff<T>::is_empty() const
{
	return !(*char_bin);
}

template <const int T>
bool cbuff<T>::operator==(const cbuff &other) const 
{ 
  return hash == other.hash; 
}

template <const int T>
cbuff<T>& cbuff<T>::operator=(const char *in_string) 
{
  set(in_string);
  return *this;
}

template <const int T>
template <const int U, const int V>
cbuff<V> cbuff<T>::operator+(const cbuff<U> &other) const
{
	char bin[V];
	snprintf(bin, V, "%s%s", char_bin, other.str());
	return cbuff<V>(bin);
}

template <const int T>
template <const int U>
cbuff<T>& cbuff<T>::operator+=(const cbuff<U> &other)
{
	char bin[T];
	snprintf(bin, T, "%s%s", char_bin, other.str());
	set(bin);
	return *this;
}

template <const int T>
cbuff<T> cbuff<T>::operator+(const char* in_string) const
{
	char bin[T];
	snprintf(bin, T, "%s%s", char_bin, in_string);
	return cbuff<T>(bin);
}

template <const int T>
cbuff<T>& cbuff<T>::operator+=(const char* in_string)
{
	char bin[T];
	snprintf(bin, T, "%s%s", char_bin, in_string);
	set(bin);
	return *this;
}

template <const int T>
template <const int U>
cbuff<T>::cbuff(const cbuff<U> &other)
{
  set(other.str());
}

template <const int T>
template <const int U>
cbuff<T>& cbuff<T>::operator=(const cbuff<U> &in_cbuff) 
{
  set(in_cbuff.str());
  return *this;
}

template <const int T>
bool cbuff<T>::operator<(const cbuff &other) const
{ 
  return hash < other.hash;
}

template <const int T>
void cbuff<T>::set(const char *in_string) 
{
	char bin[T];
	snprintf(bin, T, "%s", in_string);
	snprintf(char_bin, T, "%s", bin);
  hash = StrLib::get_char_hash(char_bin);
}

template <const int T>
int cbuff<T>::find(const char *in_string) const
{
  if(!(*in_string)) return -1;

  const char *str_ptr = char_bin;
  unsigned idx = 0;
  while(*str_ptr){
    const char *token = in_string;
    while(*token){
      if(*token == *str_ptr) return idx;
      token++;
    }
    idx++;
    str_ptr++;
  }
  return -1;
}

template <const int T>
void cbuff<T>::replace(const char search_token, const char replace_token)
{
  char *char_ptr = char_bin;
  
  while(*char_ptr){
    if(*char_ptr == search_token) *char_ptr = replace_token;
    char_ptr++;
  }
	hash = StrLib::get_char_hash(char_bin);
}

template <const int T>
const char* cbuff<T>::str(const unsigned index) const
{
  return (index < T) ? char_bin + index : nullptr;
}

template <const int T>
cbuff<T> cbuff<T>::trim(const unsigned index, const unsigned length) const
{
  char bin[T];
	const unsigned limit = length < T ? length + 1 : T;
  snprintf(bin, (length == 0) ? T : limit, "%s", char_bin);
  return cbuff<T>(bin);
}

template <const int T>
size_t cbuff<T>::gethash() const
{
  return hash;
}

// http://www.cse.yorku.ca/~oz/hash.html
size_t StrLib::get_char_hash(const char *input)
{
  size_t h = 5381;
  int c;
  while ((c = *input++)) h = ((h << 5) + h) + c; // magic sauce
  return h;
}

dd_array<unsigned> StrLib::tokenize(const char *input, const char *delimeters)
{
  dd_array<unsigned> output;
  unsigned token_indices[1000]; // assuming and maximum amount of 1000 hits
  const unsigned max_index = sizeof(token_indices)/sizeof(unsigned);
  for(unsigned i = 0; i < max_index; i++) token_indices[i] = 0;

  unsigned found_tokens = 0, curr_location = 0;
  while (*(input + curr_location))
  {
    const char *token = delimeters;
    while(*token)
    {
      if (*(input + curr_location) == *token)
      {
        token_indices[found_tokens] = curr_location;
        found_tokens++;
        break;
      }
      token++;
    }
    curr_location++;
  }

  output.resize(found_tokens);
  DD_FOREACH(unsigned, idx, output) {
    *idx.ptr = token_indices[idx.i];
  }
  
  return output;
}

template <const unsigned T>
dd_array<cbuff<T>> StrLib::tokenize2(const char *input, const char *delimeters)
{
  dd_array<cbuff<T>> output;
  char char_bin[T];
  unsigned token_indices[1000]; // assuming and maximum amount of 1000 hits
  const unsigned max_index = sizeof(token_indices)/sizeof(unsigned);
  for(unsigned i = 0; i < max_index; i++) token_indices[i] = 0;

  unsigned found_tokens = 0, curr_location = 0;
  while (*(input + curr_location))
  {
    const char *token = delimeters;
    while(*token)
    {
      if (*(input + curr_location) == *token)
      {
        token_indices[found_tokens] = curr_location;
        found_tokens++;
        break;
      }
      token++;
    }
    curr_location++;
  }
  found_tokens++;

  output.resize(found_tokens);
  curr_location = 0;
  unsigned new_start_index = 0;
  while(curr_location < found_tokens - 1){
    const unsigned length = token_indices[curr_location] - new_start_index;
    snprintf(char_bin, length + 1, "%s", input + new_start_index);
    output[curr_location] = char_bin;
    new_start_index = token_indices[curr_location] + 1;
    curr_location++;
  }
  output[curr_location] = input + new_start_index;
  
  return output;
}

template struct cbuff<8>;
template dd_array<cbuff<8>> 
StrLib::tokenize2<8>(const char *input, const char *delimeters);

template struct cbuff<16>;
template dd_array<cbuff<16>> 
StrLib::tokenize2<16>(const char *input, const char *delimeters);

template struct cbuff<32>;
template dd_array<cbuff<32>> 
StrLib::tokenize2<32>(const char *input, const char *delimeters);

template struct cbuff<64>;
template dd_array<cbuff<64>> 
StrLib::tokenize2<64>(const char *input, const char *delimeters);

template struct cbuff<128>;
template dd_array<cbuff<128>> 
StrLib::tokenize2<128>(const char *input, const char *delimeters);

template struct cbuff<256>;
template dd_array<cbuff<256>> 
StrLib::tokenize2<256>(const char *input, const char *delimeters);

template struct cbuff<512>;
template dd_array<cbuff<512>> 
StrLib::tokenize2<512>(const char *input, const char *delimeters);

template struct cbuff<1024>;
template dd_array<cbuff<1024>> 
StrLib::tokenize2<1024>(const char *input, const char *delimeters);

EXPLICIT_INST(8)
EXPLICIT_INST(16)
EXPLICIT_INST(32)
EXPLICIT_INST(64)
EXPLICIT_INST(128)
EXPLICIT_INST(256)
EXPLICIT_INST(512)
EXPLICIT_INST(1024)

#define EXPLICIT_ADD_8(SIZE)\
  template cbuff<8> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<16> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<32> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<64> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

#define EXPLICIT_ADD_16(SIZE) \
  template cbuff<16> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<16> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<32> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<64> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

#define EXPLICIT_ADD_32(SIZE) \
  template cbuff<32> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<32> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<32> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<64> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

#define EXPLICIT_ADD_64(SIZE) \
  template cbuff<64> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<64> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<64> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<64> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

#define EXPLICIT_ADD_128(SIZE) \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<128> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

#define EXPLICIT_ADD_256(SIZE) \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<256> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

#define EXPLICIT_ADD_512(SIZE) \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<512> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

#define EXPLICIT_ADD_1024(SIZE) \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<8> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<16> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<32> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<64> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<128> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<256> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<512> &in_cbuff) const; \
  template cbuff<1024> cbuff<SIZE>::operator+(const cbuff<1024> &in_cbuff) const;

EXPLICIT_ADD_8(8)
EXPLICIT_ADD_16(16)
EXPLICIT_ADD_32(32)
EXPLICIT_ADD_64(64)
EXPLICIT_ADD_128(128)
EXPLICIT_ADD_256(256)
EXPLICIT_ADD_512(512)
EXPLICIT_ADD_1024(1024)
