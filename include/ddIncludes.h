#pragma once

#ifndef DD_INC
#define DD_INC

/*
 * Copyright (c) 2016, Moses Adeagbo
 * All rights reserved.
 */

#include <inttypes.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <future>
#include <initializer_list>
#include <map>
#include <string>
#include <thread>
#include <typeinfo>

#ifdef WIN32
#define GLM_FORCE_CXX98
#define GLM_FORCE_CXX11
#define GLM_FORCE_CXX14
#define GLM_FORCE_PURE
#pragma warning(disable : 4201)  // removes non-standard extensions warnings
#endif

// GLM includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// Enum bitwise flags
/*
 * std::enable_if will take a boolean and a type (in this case targeting enums)
 * and only allow the functions to work if the boolean is true. For this
 * implementation, EnableBitMaskOperators takes a class and enables the
 *functions To activate, use explicit speciaization: template<> struct
 *EnableBitMaskOperators<T> { static const bool enable = true; }; (wrapped
 *explicit specialization in macro ENABLE_BITMASK_OPERATORS(TYPE) )
 */
template <typename Enum>
struct EnableBitMaskOperators {
  static const bool enable = false;
};

#define DD_BIT(x) (1 << (x))

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

/** \brief Useful world characteristics */
static const glm::vec3 world_front = glm::vec3(0.f, 0.f, -1.f);
static const glm::vec3 global_Xv3 = glm::vec3(1.f, 0.f, 0.f);
static const glm::vec3 global_Yv3 = glm::vec3(0.f, 1.f, 0.f);
static const glm::vec3 global_Zv3 = glm::vec3(0.f, 0.f, 1.f);
static const glm::vec4 global_Wv4 = glm::vec4(0.f, 0.f, 0.f, 1.f);

#endif