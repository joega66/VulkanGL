#include "TransformGizmoSystem.h"
#include <Engine/AssetManager.h>
#include <Components/StaticMeshComponent.h>
#include <Components/COutline.h>

void TransformGizmoSystem::Start(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	State = std::bind(&TransformGizmoSystem::Null, this, std::placeholders::_1);

	TranslateAxis =
	{
		ECS.CreateEntity(),
		ECS.CreateEntity(),
		ECS.CreateEntity()
	};

	auto& XAxis = ECS.GetComponent<Transform>(TranslateAxis.X);
	XAxis.Scale(glm::vec3(0.5f));
	XAxis.Translate(glm::vec3(1.0f, -1.0f, 0.0f));
	XAxis.Rotate(glm::vec3(0.0f, 0.0f, 1.0f), -90.0f);

	auto& YAxis = ECS.GetComponent<Transform>(TranslateAxis.Y);
	YAxis.Scale(glm::vec3(0.5f));

	auto& ZAxis = ECS.GetComponent<Transform>(TranslateAxis.Z);
	ZAxis.Scale(glm::vec3(0.5f));
	ZAxis.Translate(glm::vec3(0.0f, -1.0f, 1.0f));
	ZAxis.Rotate(glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);

	auto TransformGizmo = Scene.Assets.GetStaticMesh("Transform_Gizmo");

	ECS.AddComponent<StaticMeshComponent>(TranslateAxis.X, TransformGizmo);
	ECS.AddComponent<StaticMeshComponent>(TranslateAxis.Y, TransformGizmo);
	ECS.AddComponent<StaticMeshComponent>(TranslateAxis.Z, TransformGizmo);
	
	auto& MatX = ECS.AddComponent<Material>(TranslateAxis.X);
	MatX.Descriptors.Diffuse = Material::Green;

	auto& MatY = ECS.AddComponent<Material>(TranslateAxis.Y);
	MatY.Descriptors.Diffuse = Material::Blue;

	auto& MatZ = ECS.AddComponent<Material>(TranslateAxis.Z);
	MatZ.Descriptors.Diffuse = Material::Red;
}

void TransformGizmoSystem::Update(Scene& Scene)
{
	State(Scene);

	if (SelectedEntity)
	{
		DrawOutline(Scene, SelectedEntity);
	}
}

void TransformGizmoSystem::Null(Scene& Scene)
{
	if (Scene.Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		State = std::bind(&TransformGizmoSystem::Selection, this, std::placeholders::_1);
	}
}

void TransformGizmoSystem::Selection(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	// Select on key up.
	if (Scene.Input.GetKeyUp(EKeyCode::MouseLeft))
	{
		// Don't select while looking around.
		if (!(Scene.Cursor.Last == Scene.Cursor.Position))
		{
			if (SelectedEntity)
			{
				// If an entity is currently selected, go back to the translate tool.
				State = std::bind(&TransformGizmoSystem::TranslateTool, this, std::placeholders::_1);
			}
			else
			{
				State = std::bind(&TransformGizmoSystem::Null, this, std::placeholders::_1);
			}
			return;
		}

		const Ray Ray = Scene.Camera.ScreenPointToRay(Scene.Cursor.Position);

		std::vector<Entity> Hits;

		// @todo Wrap this in a helper for just returning the closest hit entity.
		for (auto Entity : ECS.GetEntities<StaticMeshComponent>())
		{
			if (Entity.GetEntityID() == TranslateAxis.X.GetEntityID() ||
				Entity.GetEntityID() == TranslateAxis.Y.GetEntityID() ||
				Entity.GetEntityID() == TranslateAxis.Z.GetEntityID())
				continue;

			if (Physics::Raycast(Scene, Ray, Entity))
			{
				Hits.push_back(Entity);
			}
		}

		// Remove the old outline.
		ECS.RemoveComponent<COutline>(SelectedEntity);

		if (!Hits.empty())
		{
			std::sort(Hits.begin(), Hits.end(), [&](auto& A, auto& B)
			{
				Transform& TransformA = ECS.GetComponent<Transform>(A);
				Transform& TransformB = ECS.GetComponent<Transform>(B);
				return glm::distance(TransformA.GetPosition(), Scene.Camera.GetPosition()) <
					glm::distance(TransformB.GetPosition(), Scene.Camera.GetPosition());
			});

			SelectedEntity = Hits.front();

			State = std::bind(&TransformGizmoSystem::TranslateTool, this, std::placeholders::_1);
		}
		else
		{
			SelectedEntity = Entity();

			State = std::bind(&TransformGizmoSystem::Null, this, std::placeholders::_1);
		}
	}
}

void TransformGizmoSystem::TranslateTool(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	if (Scene.Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Check if we're grabbing an axis.
		std::vector<Entity> Gizmo = { TranslateAxis.X, TranslateAxis.Y, TranslateAxis.Z };
		Entity ClosestHit;
		float Dist = std::numeric_limits<float>::max();

		std::for_each(Gizmo.begin(), Gizmo.end(), [&](auto& Entity)
		{
			if (Physics::Raycast(Scene, Scene.Camera.ScreenPointToRay(Scene.Cursor.Position), Entity))
			{
				Transform& Transform = ECS.GetComponent<class Transform>(Entity);
				if (const float NewDist = glm::distance(Transform.GetPosition(), Scene.Camera.GetPosition()); NewDist < Dist)
				{
					ClosestHit = Entity;
					Dist = NewDist;
				}
			}
		});
		
		if (ClosestHit == TranslateAxis.X)
		{
			State = std::bind(&TransformGizmoSystem::Translate<EAxis::X>, this, std::placeholders::_1);
		}
		else if (ClosestHit == TranslateAxis.Y)
		{
			State = std::bind(&TransformGizmoSystem::Translate<EAxis::Y>, this, std::placeholders::_1);
		}
		else if (ClosestHit == TranslateAxis.Z)
		{
			State = std::bind(&TransformGizmoSystem::Translate<EAxis::Z>, this, std::placeholders::_1);
		}
		else
		{
			// No axis hit... go back to selection.
			State = std::bind(&TransformGizmoSystem::Selection, this, std::placeholders::_1);
		}
	}
}

void TransformGizmoSystem::DrawOutline(Scene& Scene, Entity& Entity)
{
	auto& ECS = Scene.ECS;

	if (!ECS.HasComponent<COutline>(Entity))
	{
		ECS.AddComponent<COutline>(Entity);

		auto& TransformComponent = ECS.GetComponent<Transform>(Entity);

		ECS.GetComponent<Transform>(TranslateAxis.X).SetParent(&TransformComponent);
		ECS.GetComponent<Transform>(TranslateAxis.Y).SetParent(&TransformComponent);
		ECS.GetComponent<Transform>(TranslateAxis.Z).SetParent(&TransformComponent);
	}
}