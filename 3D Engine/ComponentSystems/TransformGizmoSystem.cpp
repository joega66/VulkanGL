#include "TransformGizmoSystem.h"
#include <Engine/AssetManager.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <Renderer/Scene.h>

TransformGizmoSystem::TransformGizmoSystem()
	: State(std::bind(&TransformGizmoSystem::Null, this))
{
	TranslateAxis =
	{
		GEntityManager.CreateEntity(),
		GEntityManager.CreateEntity(),
		GEntityManager.CreateEntity()
	};

	auto& XAxis = TranslateAxis.X.GetComponent<CTransform>();
	XAxis.Scale(glm::vec3(0.5f));
	XAxis.Translate(glm::vec3(1.0f, -1.0f, 0.0f));
	XAxis.Rotate(glm::vec3(0.0f, 0.0f, 1.0f), -90.0f);

	auto& YAxis = TranslateAxis.Y.GetComponent<CTransform>();
	YAxis.Scale(glm::vec3(0.5f));

	auto& ZAxis = TranslateAxis.Z.GetComponent<CTransform>();
	ZAxis.Scale(glm::vec3(0.5f));
	ZAxis.Translate(glm::vec3(0.0f, -1.0f, 1.0f));
	ZAxis.Rotate(glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);

	auto TransformGizmo = GAssetManager.GetStaticMesh("Transform-Gizmo");

	TranslateAxis.X.AddComponent<CStaticMesh>(TransformGizmo);
	TranslateAxis.Y.AddComponent<CStaticMesh>(TransformGizmo);
	TranslateAxis.Z.AddComponent<CStaticMesh>(TransformGizmo);

	auto& MatX = TranslateAxis.X.AddComponent<CMaterial>();
	MatX.Diffuse = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

	auto& MatY = TranslateAxis.Y.AddComponent<CMaterial>();
	MatY.Diffuse = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	auto& MatZ = TranslateAxis.Z.AddComponent<CMaterial>();
	MatZ.Diffuse = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

	TranslateAxis.X.GetComponent<CRenderer>().bVisible = false;
	TranslateAxis.Y.GetComponent<CRenderer>().bVisible = false;
	TranslateAxis.Z.GetComponent<CRenderer>().bVisible = false;
}

void TransformGizmoSystem::Update()
{
	State();

	if (SelectedEntity)
	{
		DrawOutline(SelectedEntity);
	}
}

void TransformGizmoSystem::Null()
{
	if (Input::GetKeyDown(Input::MouseLeft))
	{
		auto& Scene = Scene::Get();
		LastX = Scene.View.LastXPos;
		LastY = Scene.View.LastYPos;
		State = std::bind(&TransformGizmoSystem::Selection, this);
	}
}

void TransformGizmoSystem::Selection()
{
	auto& Scene = Scene::Get();

	// Select on key up
	if (Input::GetKeyUp(Input::MouseLeft))
	{
		// Don't select while looking around
		if (!(LastX == Scene.View.LastXPos && LastY == Scene.View.LastYPos))
		{
			if (SelectedEntity)
			{
				// If an entity is currently selected, go back to the translate tool.
				State = std::bind(&TransformGizmoSystem::TranslateTool, this);
			}
			else
			{
				State = std::bind(&TransformGizmoSystem::Null, this);
			}
			return;
		}

		const Ray Ray = Scene.View.ScreenPointToRay(GPlatform->GetMousePosition());

		std::vector<Entity> Hits;

		for (auto Entity : GEntityManager.GetEntities<CStaticMesh>())
		{
			if (Entity.GetEntityID() == TranslateAxis.X.GetEntityID() ||
				Entity.GetEntityID() == TranslateAxis.Y.GetEntityID() ||
				Entity.GetEntityID() == TranslateAxis.Z.GetEntityID())
				continue;

			if (Physics::Raycast(Ray, Entity))
			{
				Hits.push_back(Entity);
			}
		}

		if (!Hits.empty())
		{
			std::sort(Hits.begin(), Hits.end(), [&](auto& A, auto& B)
			{
				CTransform& TransformA = A.GetComponent<CTransform>();
				CTransform& TransformB = B.GetComponent<CTransform>();
				return glm::distance(TransformA.GetPosition(), Scene.View.Position) <
					glm::distance(TransformB.GetPosition(), Scene.View.Position);
			});

			SelectedEntity = Hits.front();

			State = std::bind(&TransformGizmoSystem::TranslateTool, this);
		}
		else
		{
			TranslateAxis.X.GetComponent<CRenderer>().bVisible = false;
			TranslateAxis.Y.GetComponent<CRenderer>().bVisible = false;
			TranslateAxis.Z.GetComponent<CRenderer>().bVisible = false;

			SelectedEntity = Entity();

			State = std::bind(&TransformGizmoSystem::Null, this);
		}
	}
}

void TransformGizmoSystem::TranslateTool()
{
	if (Input::GetKeyDown(Input::MouseLeft))
	{
		auto& Scene = Scene::Get();

		// Check if we're grabbing an axis
		std::vector<Entity> Gizmo = { TranslateAxis.X, TranslateAxis.Y, TranslateAxis.Z };
		Entity ClosestHit;
		float Dist = std::numeric_limits<float>::max();

		std::for_each(Gizmo.begin(), Gizmo.end(), [&](auto& Entity)
		{
			if (Physics::Raycast(Scene.View.ScreenPointToRay(GPlatform->GetMousePosition()), Entity))
			{
				CTransform& Transform = Entity.GetComponent<CTransform>();
				if (const float NewDist = glm::distance(Transform.GetPosition(), Scene.View.Position); NewDist < Dist)
				{
					ClosestHit = Entity;
					Dist = NewDist;
				}
			}
		});

		LastX = Scene.View.LastXPos;
		LastY = Scene.View.LastYPos;

		if (ClosestHit == TranslateAxis.X)
		{
			State = std::bind(&TransformGizmoSystem::TranslateX, this);
		}
		else if (ClosestHit == TranslateAxis.Y)
		{
			State = std::bind(&TransformGizmoSystem::TranslateY, this);
		}
		else if (ClosestHit == TranslateAxis.Z)
		{
			State = std::bind(&TransformGizmoSystem::TranslateZ, this);
		}
		else
		{
			// No axis hit... go back to selection.
			State = std::bind(&TransformGizmoSystem::Selection, this);
		}
	}
}

void TransformGizmoSystem::TranslateX()
{
	auto& Scene = Scene::Get();

	if (Input::GetKeyDown(Input::MouseLeft))
	{
		CTransform& Transform = SelectedEntity.GetComponent<CTransform>();
		glm::vec3 Position = Transform.GetPosition();
		Position.x += (Scene.View.LastXPos - LastX) / 100.0f;
		Transform.Translate(Position);
		LastX = Scene.View.LastXPos;
		LastY = Scene.View.LastYPos;
		Scene.View.bFreeze = true;
	}
	else if (Input::GetKeyUp(Input::MouseLeft))
	{
		Scene.View.bFreeze = false;
		State = std::bind(&TransformGizmoSystem::TranslateTool, this);
	}
}

void TransformGizmoSystem::TranslateY()
{
	auto& Scene = Scene::Get();

	if (Input::GetKeyDown(Input::MouseLeft))
	{
		CTransform& Transform = SelectedEntity.GetComponent<CTransform>();
		glm::vec3 Position = Transform.GetPosition();
		Position.y -= (Scene.View.LastYPos - LastY) / 100.0f;
		Transform.Translate(Position);
		LastX = Scene.View.LastXPos;
		LastY = Scene.View.LastYPos;
		Scene.View.bFreeze = true;
	}
	else if (Input::GetKeyUp(Input::MouseLeft))
	{
		Scene.View.bFreeze = false;
		State = std::bind(&TransformGizmoSystem::TranslateTool, this);
	}
}

void TransformGizmoSystem::TranslateZ()
{
	auto& Scene = Scene::Get();

	if (Input::GetKeyDown(Input::MouseLeft))
	{
		CTransform& Transform = SelectedEntity.GetComponent<CTransform>();
		glm::vec3 Position = Transform.GetPosition();
		Position.z += (Scene.View.LastXPos - LastX) / 100.0f;
		Transform.Translate(Position);
		LastX = Scene.View.LastXPos;
		LastY = Scene.View.LastYPos;
		Scene.View.bFreeze = true;
	}
	else if (Input::GetKeyUp(Input::MouseLeft))
	{
		Scene.View.bFreeze = false;
		State = std::bind(&TransformGizmoSystem::TranslateTool, this);
	}
}

void TransformGizmoSystem::DrawOutline(Entity Entity)
{
	auto& Scene = Scene::Get();
	auto& StaticMesh = Entity.GetComponent<CStaticMesh>();
	auto& Transform = Entity.GetComponent<CTransform>();

	auto ScaledUpTransform = Transform;
	ScaledUpTransform.Scale(ScaledUpTransform.GetScale() * glm::vec3(1.05f));

	StaticMesh.ForEach([&](const auto& Resource)
	{
		Scene.Stencil.Add(Entity, DepthPassDrawingPlan(Resource, Transform.LocalToWorldUniform));
		Scene.Outline.Add(Entity, OutlineDrawingPlan(Resource, ScaledUpTransform.LocalToWorldUniform));
	});

	TranslateAxis.X.GetComponent<CRenderer>().bVisible = true;
	TranslateAxis.Y.GetComponent<CRenderer>().bVisible = true;
	TranslateAxis.Z.GetComponent<CRenderer>().bVisible = true;

	TranslateAxis.X.GetComponent<CTransform>().SetParent(&Transform);
	TranslateAxis.Y.GetComponent<CTransform>().SetParent(&Transform);
	TranslateAxis.Z.GetComponent<CTransform>().SetParent(&Transform);
}