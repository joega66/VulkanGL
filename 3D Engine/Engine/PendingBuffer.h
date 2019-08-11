#pragma once
#include <Platform/Platform.h>

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