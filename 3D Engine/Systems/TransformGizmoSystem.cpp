#include "TransformGizmoSystem.h"
#include <Engine/AssetManager.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Outline.h>

void TransformGizmoSystem::Start(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	State = std::bind(&TransformGizmoSystem::Null, this, std::placeholders::_1);

	TranslateAxis =
	{
		ECS.CreateEntity(),
		ECS.CreateEntity(),
		ECS.CreateEntity()
	};

	auto& XAxis = ECS.GetComponent<Transform>(TranslateAxis.X);
	XAxis.Scale(ECS, glm::vec3(0.5f));
	XAxis.Translate(ECS, glm::vec3(1.0f, -1.0f, 0.0f));
	XAxis.Rotate(ECS, glm::vec3(0.0f, 0.0f, 1.0f), -90.0f);

	auto& YAxis = ECS.GetComponent<Transform>(TranslateAxis.Y);
	YAxis.Scale(ECS, glm::vec3(0.5f));

	auto& ZAxis = ECS.GetComponent<Transform>(TranslateAxis.Z);
	ZAxis.Scale(ECS, glm::vec3(0.5f));
	ZAxis.Translate(ECS, glm::vec3(0.0f, -1.0f, 1.0f));
	ZAxis.Rotate(ECS, glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);
	
	MaterialDescriptors DescX(Engine.Device);
	DescX.BaseColor.SetImage(Material::Green);
	const Material* MaterialX = Engine.Assets.LoadMaterial("Engine_X_Axis",
		std::make_unique<Material>(Engine.Device, DescX, EMaterialMode::Opaque, 0.0f, 0.0f)
	);
	MaterialDescriptors DescY(Engine.Device);
	DescY.BaseColor.SetImage(Material::Blue);
	const Material* MaterialY = Engine.Assets.LoadMaterial("Engine_Y_Axis",
		std::make_unique<Material>(Engine.Device, DescY, EMaterialMode::Opaque, 0.0f, 0.0f)
	);
	MaterialDescriptors DescZ(Engine.Device);
	DescZ.BaseColor.SetImage(Material::Red);
	const Material* MaterialZ = Engine.Assets.LoadMaterial("Engine_Z_Axis",
		std::make_unique<Material>(Engine.Device, DescZ, EMaterialMode::Opaque, 0.0f, 0.0f)
	);

	auto TransformGizmo = Engine.Assets.GetStaticMesh("Transform_Gizmo");

	ECS.AddComponent(TranslateAxis.X, StaticMeshComponent(TransformGizmo, MaterialX));
	ECS.AddComponent(TranslateAxis.Y, StaticMeshComponent(TransformGizmo, MaterialY));
	ECS.AddComponent(TranslateAxis.Z, StaticMeshComponent(TransformGizmo, MaterialZ));
}

void TransformGizmoSystem::Update(Engine& Engine)
{
	State(Engine);

	if (Engine.ECS.IsValid(SelectedEntity))
	{
		DrawOutline(Engine, SelectedEntity);
	}
}

void TransformGizmoSystem::Null(Engine& Engine)
{
	if (Engine._Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		State = std::bind(&TransformGizmoSystem::Selection, this, std::placeholders::_1);
	}
}

void TransformGizmoSystem::Selection(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	// Select on key up.
	if (Engine._Input.GetKeyUp(EKeyCode::MouseLeft))
	{
		// Don't select while looking around.
		if (!(Engine._Cursor.Last == Engine._Cursor.Position))
		{
			if (ECS.IsValid(SelectedEntity))
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

		const Ray Ray = Engine.Camera.ScreenPointToRay(Engine._Cursor.Position);

		std::vector<Entity> Hits;
		for (auto Entity : ECS.GetEntities<StaticMeshComponent>())
		{
			if (Entity.GetEntityID() == TranslateAxis.X.GetEntityID() ||
				Entity.GetEntityID() == TranslateAxis.Y.GetEntityID() ||
				Entity.GetEntityID() == TranslateAxis.Z.GetEntityID())
				continue;

			if (Physics::Raycast(Engine.ECS, Ray, Entity))
			{
				Hits.push_back(Entity);
			}
		}

		// Remove the old outline.
		ECS.RemoveComponent<Outline>(SelectedEntity);

		if (!Hits.empty())
		{
			std::sort(Hits.begin(), Hits.end(), [&](auto& A, auto& B)
			{
				Transform& TransformA = ECS.GetComponent<Transform>(A);
				Transform& TransformB = ECS.GetComponent<Transform>(B);
				return glm::distance(TransformA.GetPosition(), Engine.Camera.GetPosition()) <
					glm::distance(TransformB.GetPosition(), Engine.Camera.GetPosition());
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

void TransformGizmoSystem::TranslateTool(Engine& Engine)
{
	auto& ECS = Engine.ECS;

	if (Engine._Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Check if we're grabbing an axis.
		std::vector<Entity> Gizmo = { TranslateAxis.X, TranslateAxis.Y, TranslateAxis.Z };
		Entity ClosestHit;
		float Dist = std::numeric_limits<float>::max();

		std::for_each(Gizmo.begin(), Gizmo.end(), [&](auto& Entity)
		{
			if (Physics::Raycast(Engine.ECS, Engine.Camera.ScreenPointToRay(Engine._Cursor.Position), Entity))
			{
				Transform& Transform = ECS.GetComponent<class Transform>(Entity);
				if (const float NewDist = glm::distance(Transform.GetPosition(), Engine.Camera.GetPosition()); NewDist < Dist)
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

void TransformGizmoSystem::DrawOutline(Engine& Engine, Entity& Entity)
{
	auto& ECS = Engine.ECS;

	if (!ECS.HasComponent<Outline>(Entity))
	{
		ECS.AddComponent(Entity, Outline());
		ECS.GetComponent<Transform>(TranslateAxis.X).SetParent(ECS, Entity);
		ECS.GetComponent<Transform>(TranslateAxis.Y).SetParent(ECS, Entity);
		ECS.GetComponent<Transform>(TranslateAxis.Z).SetParent(ECS, Entity);
	}
}