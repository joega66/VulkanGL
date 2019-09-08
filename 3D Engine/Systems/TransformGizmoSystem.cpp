#include "TransformGizmoSystem.h"
#include <Engine/AssetManager.h>
#include <Components/CRenderer.h>
#include <Components/CStaticMesh.h>
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

	auto& XAxis = ECS.GetComponent<CTransform>(TranslateAxis.X);
	XAxis.Scale(glm::vec3(0.5f));
	XAxis.Translate(glm::vec3(1.0f, -1.0f, 0.0f));
	XAxis.Rotate(glm::vec3(0.0f, 0.0f, 1.0f), -90.0f);

	auto& YAxis = ECS.GetComponent<CTransform>(TranslateAxis.Y);
	YAxis.Scale(glm::vec3(0.5f));

	auto& ZAxis = ECS.GetComponent<CTransform>(TranslateAxis.Z);
	ZAxis.Scale(glm::vec3(0.5f));
	ZAxis.Translate(glm::vec3(0.0f, -1.0f, 1.0f));
	ZAxis.Rotate(glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);

	auto TransformGizmo = GAssetManager.GetStaticMesh("TransformGizmo");

	ECS.AddComponent<CStaticMesh>(TranslateAxis.X, TransformGizmo);
	ECS.AddComponent<CStaticMesh>(TranslateAxis.Y, TransformGizmo);
	ECS.AddComponent<CStaticMesh>(TranslateAxis.Z, TransformGizmo);
	
	auto& MatX = ECS.AddComponent<CMaterial>(TranslateAxis.X);
	MatX.Diffuse = CMaterial::Green;

	auto& MatY = ECS.AddComponent<CMaterial>(TranslateAxis.Y);
	MatY.Diffuse = CMaterial::Blue;

	auto& MatZ = ECS.AddComponent<CMaterial>(TranslateAxis.Z);
	MatZ.Diffuse = CMaterial::Red;

	ECS.GetComponent<CRenderer>(TranslateAxis.X).bVisible = false;
	ECS.GetComponent<CRenderer>(TranslateAxis.Y).bVisible = false;
	ECS.GetComponent<CRenderer>(TranslateAxis.Z).bVisible = false;
}

void TransformGizmoSystem::Update(Scene& Scene)
{
	State(Scene);

	if (SelectedEntity)
	{
		DrawOutline(Scene, SelectedEntity);
	}
}

void TransformGizmoSystem::Null(Scene&)
{
	if (Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		State = std::bind(&TransformGizmoSystem::Selection, this, std::placeholders::_1);
	}
}

void TransformGizmoSystem::Selection(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	// Select on key up.
	if (Input.GetKeyUp(EKeyCode::MouseLeft))
	{
		// Don't select while looking around.
		if (!(Cursor.Last == Cursor.Position))
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

		const Ray Ray = Scene.View.ScreenPointToRay(Cursor.Position);

		std::vector<Entity> Hits;

		// @todo Wrap this in a helper for just returning the closest hit entity.
		for (auto Entity : ECS.GetEntities<CStaticMesh>())
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
				CTransform& TransformA = ECS.GetComponent<CTransform>(A);
				CTransform& TransformB = ECS.GetComponent<CTransform>(B);
				return glm::distance(TransformA.GetPosition(), Scene.View.GetPosition()) <
					glm::distance(TransformB.GetPosition(), Scene.View.GetPosition());
			});

			SelectedEntity = Hits.front();

			State = std::bind(&TransformGizmoSystem::TranslateTool, this, std::placeholders::_1);
		}
		else
		{
			ECS.GetComponent<CRenderer>(TranslateAxis.X).bVisible = false;
			ECS.GetComponent<CRenderer>(TranslateAxis.Y).bVisible = false;
			ECS.GetComponent<CRenderer>(TranslateAxis.Z).bVisible = false;

			SelectedEntity = Entity();

			State = std::bind(&TransformGizmoSystem::Null, this, std::placeholders::_1);
		}
	}
}

void TransformGizmoSystem::TranslateTool(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	if (Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Check if we're grabbing an axis.
		std::vector<Entity> Gizmo = { TranslateAxis.X, TranslateAxis.Y, TranslateAxis.Z };
		Entity ClosestHit;
		float Dist = std::numeric_limits<float>::max();

		std::for_each(Gizmo.begin(), Gizmo.end(), [&](auto& Entity)
		{
			if (Physics::Raycast(Scene, Scene.View.ScreenPointToRay(Cursor.Position), Entity))
			{
				CTransform& Transform = ECS.GetComponent<CTransform>(Entity);
				if (const float NewDist = glm::distance(Transform.GetPosition(), Scene.View.GetPosition()); NewDist < Dist)
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

		ECS.GetComponent<CRenderer>(TranslateAxis.X).bVisible = true;
		ECS.GetComponent<CRenderer>(TranslateAxis.Y).bVisible = true;
		ECS.GetComponent<CRenderer>(TranslateAxis.Z).bVisible = true;

		auto& Transform = ECS.GetComponent<CTransform>(Entity);

		ECS.GetComponent<CTransform>(TranslateAxis.X).SetParent(&Transform);
		ECS.GetComponent<CTransform>(TranslateAxis.Y).SetParent(&Transform);
		ECS.GetComponent<CTransform>(TranslateAxis.Z).SetParent(&Transform);
	}
}