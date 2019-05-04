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

template<typename T>
using UniqueRef = std::unique_ptr<T>;

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ...RefArgs>
UniqueRef<T> MakeUniqueRef(RefArgs& ...Args)
{
	return std::make_unique<T>(Args...);
}

template<typename T, typename ...RefArgs>
Ref<T> MakeRef(RefArgs&& ...Args)
{
	return std::make_shared<T>(Args...);
}

#define CLASS(Class) \
	using Class ## Ref = Ref<Class>;

#define CAST(FromType, ToType) ToType ## Ref ResourceCast(FromType ## Ref From) { return std::static_pointer_cast<ToType>(From); }

class IPlatform
{
public:	
	virtual std::string GetPlatformName() = 0;

	// Window
	virtual void OpenWindow(int32 Width, int32 Height) = 0;
	virtual bool WindowShouldClose() = 0;
	virtual void PollEvents() = 0;
	virtual void MouseState(class Cursor& Cursor) = 0;

	// File I/O
	std::string FileRead(const std::string& Filename) const;
	void FileDelete(const std::string& Filename) const;
	void FileRename(const std::string& Old, const std::string& New) const;
	bool FileExists(const std::string& Filename) const;

	// Logging
	void WriteLog(const std::string& Log);
	void WriteLog(const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	void WriteLog(const std::string& Expression, const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	static std::string SanitizeFile(const std::string& file);
	static std::string FormatString(std::string format, ...);

	// Processes
	virtual void ForkProcess(const std::string& ExePath, const std::string& CmdArgs) const = 0;

	// Memory
	void Memcpy(void* Dst, const void* Src, size_t Size);

	// Loading
	uint8* LoadImage(const std::string& Filename, int32& Width, int32& Height, int32& NumChannels);
	void FreeImage(uint8* Pixels);

	// Sets OS-controlled state.
	void EndFrame();
};

CLASS(IPlatform);

extern IPlatformRef GPlatform;

// Print.
#define print(Fmt, ...) \
	GPlatform->WriteLog(IPlatform::FormatString(Fmt, __VA_ARGS__)); \

// Print and crash if expression fails.
#define check(Expression, Fmt, ...) \
	((Expression))? ((void)0) : [&] () { GPlatform->WriteLog(#Expression, __FILE__, __func__, __LINE__, IPlatform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }();

// Print error and crash.
#define fail(Fmt, ...) \
	if (true) \
	{ \
		GPlatform->WriteLog(__FILE__, __func__, __LINE__, IPlatform::FormatString(Fmt, __VA_ARGS__)); \
		throw std::runtime_error(""); \
	} \

#define signal_unimplemented() fail("UNIMPLEMENTED") \

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

	fail("Key not found.");
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
constexpr std::size_t ArrayLength(T(&)[N])
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

template<typename T>
class PendingBuffer
{
public:
	PendingBuffer(std::function<void(T)>&& Deallocator)
		: Deallocator(std::forward<std::function<void(T)>>(Deallocator))
	{
	}

	~PendingBuffer()
	{
		Destroy();
	}

	void Push(const T& Resource)
	{
		Resources.push_back(Resource);
	}

	T& Get()
	{
		return Resources.back();
	}

	void Destroy()
	{
		std::for_each(Resources.begin(), Resources.end(), Deallocator);
		Resources.clear();
	}

	uint32 Size()
	{
		return Resources.size();
	}

private:
	const std::function<void(T)> Deallocator;
	std::list<T> Resources;
};