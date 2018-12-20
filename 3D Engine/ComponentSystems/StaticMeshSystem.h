#pragma once
#include "ComponentSystem.h"

class StaticMeshSystem : public ComponentSystem
{
public:
	virtual void RenderUpdate(class Scene* Scene) override;
};