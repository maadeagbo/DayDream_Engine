#pragma once

/*
 * Copyright (c) 2016, Moses Adeagbo
 * All rights reserved.
 */

/*-----------------------------------------------------------------------------
*
*	DD_Types:
*		- Typesdefs and enums for engine use
*	DD_Event:
*		- Defines the message objects
*
-----------------------------------------------------------------------------*/

#include <inttypes.h>
#include <inttypes.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <future>
#include <map>
#include <string>
#include <thread>
#include <typeinfo>

// simple template container library
#include "DD_Container.h"
// simple string library
#include "DD_String.h"
// lua bindings
#include "DD_LuaHooks.h"

// OpenGL includes
#include <gl_core_4_3.h>

// GLM includes
#include <glm/fwd.hpp>
#ifdef _WIN32
#define GLM_FORCE_CXX98
#define GLM_FORCE_CXX11
#define GLM_FORCE_CXX14  // removes non-standard extensions warnings
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// Bullet Physics include
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

typedef std::uint_fast8_t u8;    // 8-bit unsigned int for fast access
typedef std::uint_fast32_t u32;  // 32-bit unsigned int for high precision
typedef std::uint_fast64_t u64;  // 64-bit unsigned int for high precision
typedef std::int_fast64_t I64;   // 64-bit signed int for high precision

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

/// \brief Cotainer for scene graph
struct ParentInfo {
  /// \brief Engine identifier for parent agent
  size_t parent_id;
  /// \brief Boolean to set if object is parented
  bool parent_set = false;
};
