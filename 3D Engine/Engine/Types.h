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

// Reference: boost::hash_combine
template<typename SeedType, typename T>
inline void HashCombine(SeedType& Seed, const T& V)
{
	std::hash<T> Hasher;
	Seed ^= Hasher(V) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
}

template<typename T, std::size_t N>
constexpr std::size_t ARRAY_SIZE(T(&)[N])
{
	return N;
}

#define ENABLE_BITWISE_OPERATORS(T) \
inline constexpr T operator~ (T a) { return (T)~(int)a; } \
inline constexpr T operator| (T a, T b) { return (T)((int)a | (int)b); } \
inline constexpr T operator& (T a, T b) { return (T)((int)a & (int)b); } \
inline constexpr T operator^ (T a, T b) { return (T)((int)a ^ (int)b); } \
inline constexpr T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); } \
inline constexpr T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); } \
inline constexpr T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); } \

template<typename EnumClass>
inline bool Any(EnumClass EnumTrait)
{
	return EnumTrait != EnumClass::None;
}

template<typename T>
inline T DivideAndRoundUp(const T& Num, const T& Denom)
{
	return (Num + Denom - 1) / Denom;
}