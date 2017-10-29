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

#include <cstdint>
#include <cstdio>
#include <string>
#include <inttypes.h>
#include <functional>
#include <future>
#include <thread>
#include <chrono>
#include <cmath>
#include <typeinfo>
#include <algorithm>
#include <map>

#include "DD_Container.h"
#include "DD_String.h"
#include "DD_EventTypes.h"

#include <gl_core_4_3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

typedef std::uint_fast8_t u8; // 8-bit unsigned int for fast access
typedef std::uint_fast32_t u32; // 32-bit unsigned int for high precision
typedef std::uint_fast64_t u64; // 64-bit unsigned int for high precision
typedef std::int_fast64_t  I64; // 64-bit signed int for high precision

// Enum bitwise flags
/*
* std::enable_if will take a boolean and a type (in this case targeting enums)
* and only allow the functions to work if the boolean is true. For this
* implementation, EnableBitMaskOperators takes a class and enables the functions
* To activate, use explicit speciaization:
* 	template<>
*	struct EnableBitMaskOperators<T> { static const bool enable = true; };
* (wrapped explicit specialization in macro ENABLE_BITMASK_OPERATORS(TYPE) )
*/
template<typename Enum>
struct EnableBitMaskOperators
{
	static const bool enable = false;
};

#define ENABLE_BITMASK_OPERATORS(TYPE) \
template<> \
struct EnableBitMaskOperators<TYPE>	\
{ \
    static const bool enable = true; \
};

// bitwise Or
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
		);
}

// bitwise Or-Eq
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator |=(Enum &lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum> (
		static_cast<underlying>(lhs) |
		static_cast<underlying>(rhs)
		);
	return lhs;
}

// bitwise And
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator &(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum> (
		static_cast<underlying>(lhs) &
		static_cast<underlying>(rhs)
		);
}