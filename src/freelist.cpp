#include "freelist.h"
#include <bitset>

void freelist::initialize(const unsigned size) {
  size_t array_size = size < FREELIST_MAX - 1 ? size : FREELIST_MAX - 1;
  list.resize(array_size);

  // set bits
  for (uint32_t i = 0; i < (uint32_t)list.size(); i++) {
    // set index bit
    list[i] = ((i - 1) << 1);
    // set boolean bit
    list[i] &= (~(0)) << 1;
  }
  nxt_free = (uint32_t)list.size() - 1;
}

bool freelist::nxt_free_slot(uint32_t &next_idx) {
  // check for full list index
  if (!is_not_empty()) {
    return false;
  }

  // set nxt_free value and bit flag
  next_idx = nxt_free;
  list[nxt_free] |= 1;
  nxt_free = list[nxt_free] >> 1;
  return true;
}

bool freelist::release_slot(const unsigned idx) {
  // skip if node doesn't exist or is already free
  if ((idx > (uint32_t)list.size() - 1) || is_free(idx)) {
    return false;
  }

  // set index and boolean bit
  list[idx] = nxt_free << 1;
  list[idx] &= (~(0)) << 1;
  nxt_free = idx;
  return true;
}

uint32_t freelist::remaining_slots() {
  uint32_t f_slots = 0;
  for (size_t i = 0; i < list.size(); i++) {
    if ((list[i] & 1U) == 0) {
      f_slots++;
    }
  }
  return f_slots;
}

void freelist::print_freelist() {
  for (unsigned i = 0; i < (unsigned)list.size(); i++) {
    printf("\t<%u> = ptr::%u | flag::%s | binary::%s\n", i, list[i] >> 1,
           (list[i] & 1U) == 0 ? "true" : "false",
           std::bitset<32>(list[i]).to_string().c_str());
  }
}