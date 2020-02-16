#pragma once
#include <ECS/System.h>
#include <ECS/EntityManager.h>
#include <Components/Transform.h>
#include <Engine/Engine.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>
#include <Engine/Scene.h>

class TransformGizmoSystem : public ISystem
{
	SYSTEM(TransformGizmoSystem);
public:
	virtual void Start(Engine& Engine) final;
	virtual void Update(Engine& Engine) final;
	
private:
	// X,Y,Z axes of the translate gizmo. 
	struct
	{
		Entity X, Y, Z;
	} TranslateAxis;

	// Currently selected entity.
	Entity SelectedEntity;

	// The current state of the gizmo system.
	std::function<void(Engine& Engine)> State;

	// States.
	void Null(Engine& Engine);
	void Selection(Engine& Engine);
	void TranslateTool(Engine& Engine);

	enum class EAxis
	{
		X,
		Y,
		Z
	};

	template<EAxis Axis>
	void Translate(Engine& Engine)
	{
		auto& ECS = Engine.ECS;

		if (Engine._Input.GetKeyDown(EKeyCode::MouseLeft))
		{
			Engine._Cursor.Mode = ECursorMode::Normal;

			Transform& Transform = ECS.GetComponent<class Transform>(SelectedEntity);
			glm::vec3 Position = Transform.GetPosition();
			Ray Ray0 = Engine.Camera.ScreenPointToRay(Engine._Cursor.Last);
			Ray Ray1 = Engine.Camera.ScreenPointToRay(Engine._Cursor.Position);

			if constexpr (Axis == EAxis::X)
			{
				if (float T0, T1; Physics::Raycast(Engine.ECS, Ray0, TranslateAxis.X, T0) && Physics::Raycast(Engine.ECS, Ray1, TranslateAxis.X, T1))
				{
					Position.x += Ray1.Intersect(T1).x - Ray0.Intersect(T0).x;
				}
			}
			else if constexpr (Axis == EAxis::Y)
			{
				if (float T0, T1; Physics::Raycast(Engine.ECS, Ray0, TranslateAxis.Y, T0) && Physics::Raycast(Engine.ECS, Ray1, TranslateAxis.Y, T1))
				{
					Position.y += Ray1.Intersect(T1).y - Ray0.Intersect(T0).y;
				}
			}
			else if constexpr (Axis == EAxis::Z)
			{
				if (float T0, T1; Physics::Raycast(Engine.ECS, Ray0, TranslateAxis.Z, T0) && Physics::Raycast(Engine.ECS, Ray1, TranslateAxis.Z, T1))
				{
					Position.z += Ray1.Intersect(T1).z - Ray0.Intersect(T0).z;
				}
			}

			Transform.Translate(ECS, Position);
			Engine.Camera.bFreeze = true;
		}
		else if (Engine._Input.GetKeyUp(EKeyCode::MouseLeft))
		{
			State = std::bind(&TransformGizmoSystem::TranslateTool, this, std::placeholders::_1);
			Engine.Camera.bFreeze = false;
		}
	}

	void DrawOutline(Engine& Engine, Entity& Entity);
};