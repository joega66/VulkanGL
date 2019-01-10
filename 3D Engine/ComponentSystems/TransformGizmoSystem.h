#pragma once
#include "ComponentSystem.h"
#include <Components/Entity.h>

class TransformGizmoSystem : public ComponentSystem
{
public:
	struct
	{
		Entity X, Y, Z;
	} TranslateAxis;

	Entity SelectedEntity;
	float LastX, LastY;

	TransformGizmoSystem();
	virtual void Update() final;

	// The current state of the gizmo system.
	std::function<void()> State;
	// States.
	void Null();
	void Selection();
	void TranslateTool();
	void TranslateX();
	void TranslateY();
	void TranslateZ();
	
private:
	void DrawOutline(Entity Entity);
};