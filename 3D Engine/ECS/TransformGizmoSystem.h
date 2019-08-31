#pragma once
#include "System.h"
#include <Components/Entity.h>
#include <Components/CTransform.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <Renderer/Scene.h>

class TransformGizmoSystem : public ISystem
{
	SYSTEM(TransformGizmoSystem);
public:
	virtual void Start() final;
	virtual void Update() final;
	
private:
	// X,Y,Z axes of the translate gizmo. 
	struct
	{
		Entity X, Y, Z;
	} TranslateAxis;

	// Currently selected entity.
	Entity SelectedEntity;

	// The current state of the gizmo system.
	std::function<void()> State;

	// States.
	void Null();
	void Selection();
	void TranslateTool();

	enum class EAxis
	{
		X,
		Y,
		Z
	};

	template<EAxis Axis>
	void Translate()
	{
		auto& Scene = Scene::Get();

		if (Input.GetKeyDown(EKeyCode::MouseLeft))
		{
			Cursor.Mode = ECursorMode::Normal;

			CTransform& Transform = SelectedEntity.GetComponent<CTransform>();
			glm::vec3 Position = Transform.GetPosition();
			Ray Ray0 = Scene.View.ScreenPointToRay(Cursor.Last);
			Ray Ray1 = Scene.View.ScreenPointToRay(Cursor.Position);

			if constexpr (Axis == EAxis::X)
			{
				if (float T0, T1; Physics::Raycast(Ray0, TranslateAxis.X, T0) && Physics::Raycast(Ray1, TranslateAxis.X, T1))
				{
					Position.x += Ray1.Intersection(T1).x - Ray0.Intersection(T0).x;
				}
			}
			else if constexpr (Axis == EAxis::Y)
			{
				if (float T0, T1; Physics::Raycast(Ray0, TranslateAxis.Y, T0) && Physics::Raycast(Ray1, TranslateAxis.Y, T1))
				{
					Position.y += Ray1.Intersection(T1).y - Ray0.Intersection(T0).y;
				}
			}
			else if constexpr (Axis == EAxis::Z)
			{
				if (float T0, T1; Physics::Raycast(Ray0, TranslateAxis.Z, T0) && Physics::Raycast(Ray1, TranslateAxis.Z, T1))
				{
					Position.z += Ray1.Intersection(T1).z - Ray0.Intersection(T0).z;
				}
			}

			Transform.Translate(Position);
			Scene.View.bFreeze = true;
		}
		else if (Input.GetKeyUp(EKeyCode::MouseLeft))
		{
			State = std::bind(&TransformGizmoSystem::TranslateTool, this);
			Scene.View.bFreeze = false;
		}

	}

	void DrawOutline(Entity Entity);
};