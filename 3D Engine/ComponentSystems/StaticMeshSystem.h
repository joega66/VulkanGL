#pragma once
#include "ComponentSystem.h"

class StaticMeshSystem : public ComponentSystem
{
public:
	StaticMeshSystem();
	virtual void RenderUpdate() override;
	virtual void OnRemove(std::type_index Type, const Entity& Entity) override;
};