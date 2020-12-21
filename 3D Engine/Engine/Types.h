#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <array>

#define ENTRY(Key, Value) { Key, Value },

using uint8 = uint8_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;
using float64 = double;

#define ENABLE_BITWISE_OPERATORS(T) \
inline constexpr T operator~ (T a) { return (T)~(int)a; } \
inline constexpr T operator| (T a, T b) { return (T)((int)a | (int)b); } \
inline constexpr T operator& (T a, T b) { return (T)((int)a & (int)b); } \
inline constexpr T operator^ (T a, T b) { return (T)((int)a ^ (int)b); } \
inline constexpr T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); } \
inline constexpr T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); } \
inline constexpr T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); } \

template<typename EnumClass>
inline bool Any(EnumClass enumTrait)
{
	return enumTrait != EnumClass::None;
}

template<typename T>
inline T DivideAndRoundUp(const T& num, const T& denom)
{
	return (num + denom - 1) / denom;
}