#pragma once
#include <Platform/Platform.h>

template<typename T, bool bNeedsRenderUpdate = false>
struct Component
{
private:
	friend class Entity;
	static constexpr bool bNeedsRenderUpdate = bNeedsRenderUpdate;
};