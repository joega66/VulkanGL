#pragma once
#include "ComponentSystem.h"
#include <Components/Entity.h>

class TransformGizmoSystem : public ComponentSystem
{
public:
	struct
	{
		Entity X, Y, Z;
	} TransformWidget;

	Entity SelectedEntity;
	float LastX, LastY;
	bool bFirstPress = true;

	TransformGizmoSystem();
	virtual void Update() final;

private:
	void DrawOutline(Entity Entity);
};