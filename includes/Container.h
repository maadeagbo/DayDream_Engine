#pragma once

#define DAYDREAM_CONTAINERS
#ifdef DAYDREAM_CONTAINERS

#include <algorithm>
#include "Pow2Assert.h"

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	dd_array:
*		- array container
*			- can be accessed w/ []
*			- can be resized
*			- has validation check
*			- reports n size & n byte size
*	dd_2Darray:
*		- 2D array container
*			- can be accessed w/ [][]
*			- can be resized
*			- has validation check
*			- reports n size & n byte size
*	TODO:
*
*	Uses POW_ASSERT
-----------------------------------------------------------------------------*/

// Array container used for Day Dream engine
template <class T>
class dd_array {
 public:
  // ctor
  dd_array(const size_t size = 0) : _size(size) {
    if (size != 0) {
      m_data = new T[size]();
      POW2_VERIFY_MSG(m_data != nullptr, "New operator failed :: 1D", 0);
    } else {
      m_data = nullptr;
    }
  }
  // dtor
  ~dd_array() {
    if (_size > 0) {
      delete[] m_data;
    }
  }
  // copy ctor
  dd_array(const dd_array& other)
      : _size(other._size), m_data(new T[other._size]()) {
    std::copy(other.m_data, other.m_data + _size, m_data);
  }

  // set size
  bool resize(const size_t size) {
    if (_size > 0) {
      delete[] m_data;
      _size = 0;
    }
    if (size != 0) {
      _size = size;
      m_data = new T[size]();
    }
    return isValid();
  }

  // returns T from 1D array
  T& operator[](const size_t FirstIndex) {
    POW2_VERIFY_MSG(FirstIndex < _size, "Index out of bounds :: 1D", 0);
    return m_data[FirstIndex];
  }

  // returns const T from 1D array
  T& operator[](const size_t FirstIndex) const {
    POW2_VERIFY_MSG(FirstIndex < _size, "Index out of bounds :: 1D", 0);
    return m_data[FirstIndex];
  }

  // copying
  void operator=(const dd_array& other) {
    if (other._size == 0) {
      return;  // nothing in the second array
    }
    if (_size == 0) {  // nothing in this array
      _size = other._size;
      m_data = new T[_size]();
    }

    if (_size >= other._size) {  // other array is smaller
      std::copy(other.m_data, other.m_data + other._size, m_data);
    } else {  // other array is bigger or equal to this array
      std::copy(other.m_data, other.m_data + _size, m_data);
    }
  }

  // move ctor
  dd_array(dd_array&& other) : _size(0), m_data(nullptr) {
    m_data = other.m_data;
    _size = other._size;

    other.m_data = nullptr;
    other._size = 0;
  }

  // move assignment
  dd_array& operator=(dd_array&& other) {
    if (this != &other) {
      if (m_data != nullptr) {
        delete[] m_data;
      }
      m_data = other.m_data;
      _size = other._size;

      other._size = 0;
      other.m_data = nullptr;
    }
    return *this;
  }

  // number of elements
  inline size_t size() const { return _size; }
  // size of data in bytes
  inline size_t sizeInBytes() const { return _size * sizeof(T); }
  // checks is data was allocated in memory
  inline bool isValid() const { return (m_data == nullptr) ? false : true; }

 private:
  size_t _size;
  T* m_data;
};

template <class T>
class dd_2Darray {
 public:
  // proxy object used to provide second brackets
  template <class R>
  class OperatorBracketHelper {
   public:
    OperatorBracketHelper(dd_2Darray<R>& Parent, size_t FirstIndex)
        : m_parent(Parent), m_firstIndex(FirstIndex) {}
    // method called for "second brackets"
    R& operator[](size_t SecondIndex) {
      return m_parent.GetElement(m_firstIndex, SecondIndex);
    }

   private:
    dd_2Darray<R>& m_parent;
    size_t m_firstIndex;
  };

  // ctor
  dd_2Darray(const size_t Row = 0, const size_t Column = 0)
      : m_row(Row), m_column(Column) {
    if (Row != 0 && Column != 0) {
      m_data = new T[(Row * Column)]();
      POW2_VERIFY_MSG(m_data != nullptr, "New operator failed :: 1D", 0);
    } else {
      m_data = nullptr;
    }
  }
  // dtor
  ~dd_2Darray() {
    if (isValid()) {
      delete[] m_data;
    }
  }

  // set size
  bool resize(const size_t Row, const size_t Column) {
    if (isValid()) {
      m_row = 0;
      m_column = 0;
      delete[] m_data;
    }
    if (Row != 0 && Column != 0) {
      m_row = Row;
      m_column = Column;
      m_data = new T[(Row * Column)]();
    }
    return isValid();
  }

  // Return a proxy object that "knows" to which container it has to ask the
  // element and which is the first index (specified in this call). Returns T
  // from 2D array
  OperatorBracketHelper<T> operator[](size_t FirstIndex) {
    return OperatorBracketHelper<T>(*this, FirstIndex);
  }

  // return 2D data
  T& GetElement(size_t FirstIndex, size_t SecondIndex) {
    POW2_VERIFY_MSG((FirstIndex * m_column + SecondIndex) < (m_column * m_row),
                    "Index out of bounds :: 2D", 0);
    return m_data[(FirstIndex * m_column) + SecondIndex];
  }
  // return const 2D data
  T& GetElement(size_t FirstIndex, size_t SecondIndex) const {
    POW2_VERIFY_MSG((FirstIndex * m_column + SecondIndex) < (m_column * m_row),
                    "Index out of bounds :: 2D", 0);
    return m_data[(FirstIndex * m_column) + SecondIndex];
  }

  // copying
  void operator=(const dd_2Darray& other) {
    if (other.m_column == 0 && other.m_row == 0) {
      return;  // nothing in the second array
    }
    if (m_row == 0 && m_column == 0) {  // nothing in this array
      m_row = other.m_row;
      m_column = other.m_column;
      m_data = new T[(m_row * m_column)]();
    }

    if (other.m_column == m_column && other.m_row == m_row) {
      std::copy(other.m_data, other.m_data + (other.m_row * other.m_column),
                m_data);
    } else {
      const size_t min_row = std::min(other.m_row, m_row);
      const size_t min_col = std::min(other.m_column, m_column);
      for (size_t i = 0; i < min_row; i++) {
        const size_t m_i = i * m_column;
        const size_t o_i = i * other.m_column;
        std::copy(other.m_data + o_i, other.m_data + o_i + min_col,
                  m_data + m_i);
      }
    }
  }

  // move ctor
  dd_2Darray(dd_2Darray&& other) : m_data(nullptr), m_row(0), m_column(0) {
    m_data = other.m_data;
    m_row = other.m_row;
    m_column = other.m_column;

    other.m_data = nullptr;
    other.m_row = 0;
    other.m_column = 0;
  }

  // move assignment
  dd_2Darray& operator=(dd_2Darray&& other) {
    if (this != &other) {
      if (m_data != nullptr) {
        delete[] m_data;
      }
      m_data = other.m_data;
      m_row = other.m_row;
      m_column = other.m_column;

      other.m_data = nullptr;
      other.m_row = 0;
      other.m_column = 0;
    }
    return *this;
  }

  // number of elements
  inline size_t size() const { return m_row * m_column; }
  // size of data in bytes
  inline size_t sizeInBytes() const { return m_row * m_column * sizeof(T); }
  inline size_t numRows() const { return m_row; }
  inline size_t numColumns() const { return m_column; }
  // checks is data was allocated in memory
  inline bool isValid() const { return (m_data == nullptr) ? false : true; }

 private:
  size_t m_row, m_column;
  T* m_data;
};

#define DD_FOREACH(TYPE, ITER, VAR, ddARRAY)                    \
  TYPE* VAR = nullptr;                                          \
  ITER = 0;                                                     \
  auto _dd_check = [&](dd_array<TYPE>& ddARRAY) {               \
    VAR = (ITER < ddARRAY.size()) ? &ddARRAY[ITER++] : nullptr; \
    return VAR;                                                 \
  };                                                            \
  while (_dd_check(ddARRAY))

#endif  // !DDAYDREAM_CONTAINERS
