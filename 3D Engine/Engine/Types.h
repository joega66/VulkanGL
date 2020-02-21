#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <vector>
#include <mutex>
#include <future>
#include <tuple>
#include <algorithm>
#include <variant>

#define ENTRY(Key, Value) { Key, Value },

using uint8 = uint8_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;
using float64 = double;

struct RID
{
	unsigned long  Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
};

template<typename K, typename V>
using HashTable = std::unordered_map<K, V>;

template<typename K, typename V>
const K& GetKey(const HashTable<K, V>& HashTable, const V& Value)
{
	auto Iter = HashTable.begin();
	while (Iter != HashTable.end())
	{
		if (Iter->second == Value)
		{
			return Iter->first;
		}
		Iter++;
	}

	throw std::runtime_error("Key not found.");
}

template<typename K, typename V>
bool HasValue(const HashTable<K, V>& HashTable, const V& Value)
{
	for (const auto&[Key, OtherValue] : HashTable)
	{
		if (Value == OtherValue)
		{
			return true;
		}
	}

	return false;
}

template<typename K, typename V>
inline const V& GetValue(const HashTable<K, V>& HashTable, const K& Key)
{
	return HashTable.at(Key);
}

template<typename ContainerType, typename ElementType>
inline bool Contains(const ContainerType& Container, const ElementType& Element)
{
	return Container.count(Element) != 0;
}

template<typename T, std::size_t N>
constexpr std::size_t ARRAY_SIZE(T(&)[N])
{
	return N;
}

template<class T> inline constexpr T operator~ (T a) { return (T)~(int)a; }
template<class T> inline constexpr T operator| (T a, T b) { return (T)((int)a | (int)b); }
template<class T> inline constexpr T operator& (T a, T b) { return (T)((int)a & (int)b); }
template<class T> inline constexpr T operator^ (T a, T b) { return (T)((int)a ^ (int)b); }
template<class T> inline constexpr T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
template<class T> inline constexpr T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
template<class T> inline constexpr T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

template<typename EnumClass>
inline bool Any(EnumClass EnumTrait)
{
	return EnumTrait != EnumClass::None;
}