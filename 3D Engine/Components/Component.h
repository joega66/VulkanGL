#pragma once
#include <Platform/Platform.h>

template<typename T>
struct Component
{
private:
	friend class Entity;
};