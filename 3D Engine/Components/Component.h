#pragma once
#include "Platform/Platform.h"

class Component
{
public:
	virtual void GameUpdate() {}
	virtual void RenderUpdate(class Scene* Scene) {}
};

CLASS(Component);