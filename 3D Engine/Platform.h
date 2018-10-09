#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <chrono>
#include <mutex>

#ifndef NDEBUG
//#define DEBUG_BUILD
#endif

#define ENTRY(Key, Value) { Key, Value },

using uint8 = uint8_t;
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

class WindowResizeListener
{
public:
	~WindowResizeListener();
	virtual void OnWindowResize(int32 WindowX, int32 WindowY) {};
};

using WindowResizeListenerRef = WindowResizeListener*;

class IPlatform
{
public:	
	virtual std::string GetPlatformName() = 0;

	// Window
	virtual void OpenWindow(int32 Width, int32 Height) = 0;
	virtual glm::ivec2 GetWindowSize() = 0;
	virtual bool WindowShouldClose() = 0;
	virtual void PollEvents() = 0;

	// File I/O
	std::string FileRead(const std::string& Filename) const;
	void FileDelete(const std::string& Filename) const;
	void FileRename(const std::string& Old, const std::string& New) const;
	bool FileExists(const std::string& Filename) const;

	// Strings
	virtual void RemoveNewlines(std::string& String) = 0;
	virtual void RemoveSpaces(std::string& String);

	void WriteLog(const std::string& Log);
	void WriteLog(const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	void WriteLog(const std::string& Expression, const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	static std::string SanitizeFile(const std::string& file);
	static std::string FormatString(std::string format, ...);

	// Processes
	virtual void ForkProcess(const std::string& ExePath, const std::string& CmdArgs) const = 0;

	void AddWindowListener(WindowResizeListenerRef WindowListener);
	void RemoveWindowListener(WindowResizeListenerRef WindowListener);
	void NotifyWindowListeners(int32 X, int32 Y);

	// Memory
	void Memcpy(void* Dst, const void* Src, size_t Size);

	// Loading
	uint8* LoadImage(const std::string& Filename, int32& Width, int32& Height, int32& NumChannels);
	void FreeImage(uint8* Pixels);

protected:
	std::list<WindowResizeListenerRef> WindowListeners;
};

CLASS(IPlatform);

extern IPlatformRef GPlatform;

// Print
#define print(Fmt, ...) \
	GPlatform->WriteLog(__FILE__, __func__, __LINE__, IPlatform::FormatString(Fmt, __VA_ARGS__)); \

// Print and crash if expression fails
#define check(Expression, Fmt, ...) \
	((Expression))? ((void)0) : [&] () { GPlatform->WriteLog(#Expression, __FILE__, __func__, __LINE__, IPlatform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }();

// Print error and crash
#define fail(Fmt, ...) \
	if (true) \
	{ \
		GPlatform->WriteLog(__FILE__, __func__, __LINE__, IPlatform::FormatString(Fmt, __VA_ARGS__)); \
		throw std::runtime_error(""); \
	} \

#define signal_unimplemented() fail("UNIMPLEMENTED") \

template<typename K, typename V>
using Map = std::unordered_map<K, V>;

template<typename K, typename V>
const K& GetKey(const Map<K, V>& Map, const V& Value)
{
	auto Iter = Map.begin();
	while (Iter != Map.end())
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
const V& GetValue(const Map<K, V>& Map, const K& Key)
{
	return Map.at(Key);
}

template<typename ContainerType, typename ElementType>
bool Contains(const ContainerType& Container, const ElementType& Element)
{
	return Container.find(Element) != Container.end();
}

template<typename T, std::size_t N>
constexpr std::size_t ARRAY_LENGTH(T(&)[N])
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

template<typename T>
struct PendingBuffer
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

class ScopedTimer
{
public:
	ScopedTimer(bool bFrequency = false)
		: bFrequency(bFrequency)
	{
		Start = std::chrono::system_clock::now();
	}

	~ScopedTimer()
	{
		auto End = std::chrono::system_clock::now();
		std::chrono::duration<float> Duration = End - Start;
		print("Elapsed time: %.3f", bFrequency ? 1 / Duration.count() : Duration.count());
	}

private:
	const bool bFrequency;
	std::chrono::system_clock::time_point Start;
};