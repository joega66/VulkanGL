#pragma once
#include <ECS/System.h>
#include <ECS/EntityManager.h>
#include <Components/Transform.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <Engine/Scene.h>

class TransformGizmoSystem : public ISystem
{
	SYSTEM(TransformGizmoSystem);
public:
	virtual void Start(Scene& Scene) final;
	virtual void Update(Scene& Scene) final;
	
private:
	// X,Y,Z axes of the translate gizmo. 
	struct
	{
		Entity X, Y, Z;
	} TranslateAxis;

	// Currently selected entity.
	Entity SelectedEntity;

	// The current state of the gizmo system.
	std::function<void(Scene& Scene)> State;

	// States.
	void Null(Scene& Scene);
	void Selection(Scene& Scene);
	void TranslateTool(Scene& Scene);

	enum class EAxis
	{
		X,
		Y,
		Z
	};

	template<EAxis Axis>
	void Translate(Scene& Scene)
	{
		auto& ECS = Scene.ECS;

		if (Scene.Input.GetKeyDown(EKeyCode::MouseLeft))
		{
			Scene.Cursor.Mode = ECursorMode::Normal;

			Transform& Transform = ECS.GetComponent<class Transform>(SelectedEntity);
			glm::vec3 Position = Transform.GetPosition();
			Ray Ray0 = Scene.Camera.ScreenPointToRay(Scene.Cursor.Last);
			Ray Ray1 = Scene.Camera.ScreenPointToRay(Scene.Cursor.Position);

			if constexpr (Axis == EAxis::X)
			{
				if (float T0, T1; Physics::Raycast(Scene, Ray0, TranslateAxis.X, T0) && Physics::Raycast(Scene, Ray1, TranslateAxis.X, T1))
				{
					Position.x += Ray1.Intersect(T1).x - Ray0.Intersect(T0).x;
				}
			}
			else if constexpr (Axis == EAxis::Y)
			{
				if (float T0, T1; Physics::Raycast(Scene, Ray0, TranslateAxis.Y, T0) && Physics::Raycast(Scene, Ray1, TranslateAxis.Y, T1))
				{
					Position.y += Ray1.Intersect(T1).y - Ray0.Intersect(T0).y;
				}
			}
			else if constexpr (Axis == EAxis::Z)
			{
				if (float T0, T1; Physics::Raycast(Scene, Ray0, TranslateAxis.Z, T0) && Physics::Raycast(Scene, Ray1, TranslateAxis.Z, T1))
				{
					Position.z += Ray1.Intersect(T1).z - Ray0.Intersect(T0).z;
				}
			}

			Transform.Translate(ECS, Position);
			Scene.Camera.bFreeze = true;
		}
		else if (Scene.Input.GetKeyUp(EKeyCode::MouseLeft))
		{
			State = std::bind(&TransformGizmoSystem::TranslateTool, this, std::placeholders::_1);
			Scene.Camera.bFreeze = false;
		}
	}

	void DrawOutline(Scene& Scene, Entity& Entity);
};