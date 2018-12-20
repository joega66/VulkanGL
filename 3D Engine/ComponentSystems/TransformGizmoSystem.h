#pragma once
#include "ComponentSystem.h"
#include "Components/Entity.h"

class TransformGizmoSystem : public ComponentSystem
{
public:
	uint64 SelectedEntity = Entity::InvalidID;
	virtual void Update(class Scene* Scene) override;
};