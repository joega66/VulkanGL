#pragma once
#include "ComponentSystem.h"
#include "Components/Entity.h"

class TransformGizmoSystem : public ComponentSystem
{
public:
	Entity SelectedEntity;
	float LastX, LastY;
	bool bFirstPress = true;

	TransformGizmoSystem();
	virtual void Update(class Scene& Scene) final;
	void RemoveOutline(Scene& Scene, Entity Entity);
};