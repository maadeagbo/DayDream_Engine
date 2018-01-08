/*
Copyright: Moses Adeagbo. Dec 23, 2017
Liscence: MIT
*/

#include "ddIncludes.h"

#define FREELIST_MAX (((uint32_t)-1) >> 1)

/// \brief Simple library to compactly keep track of free and assigned nodes
struct freelist {
  freelist() : list(0) {}

  /// \brief free list array
  dd_array<uint32_t> list;
  /// \brief next free node
  uint32_t nxt_free = 0;

  /// \brief Initialize free list.
  /// Undefined behavior if size > (FREELIST_MAX - 1)
  /// \param size Size of list (max size of 31 bit integer - 1)
  void initialize(const unsigned size);
  /// \brief Get next valid node
  /// \param next_idx Reference (uint32_t) to assign next free node to
  /// \return False if list is full (no more empty slota)
  bool nxt_free_slot(uint32_t &next_idx);
  /// \brief Free node and return to list
  /// \param idx Index of node to free
  /// \return False if node wasn't released
  bool release_slot(const unsigned idx);
  /// \brief Count the number of free slots
  /// \return # of free slots
  uint32_t remaining_slots();
  /// \brief Debug information on list contents
  void print_freelist();
  /// \brief Size of list
  inline uint32_t size() const { return (uint32_t)list.size(); }
  /// \brief Check if index if free
  /// \return True if slot is free
  inline bool is_free(uint32_t idx) const { return (list[idx] & 1U) == 0; }
  /// \brief Check is list contains any more free slots
  /// \return True if list has more slots
  inline bool is_not_empty() const { return nxt_free != ((uint32_t)-1) >> 1; }
};