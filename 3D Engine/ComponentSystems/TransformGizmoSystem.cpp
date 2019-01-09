#include "TransformGizmoSystem.h"
#include <Engine/AssetManager.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <Renderer/Scene.h>

TransformGizmoSystem::TransformGizmoSystem()
{
	TransformWidget =
	{
		GEntityManager.CreateEntity(),
		GEntityManager.CreateEntity(),
		GEntityManager.CreateEntity()
	};

	auto& XAxis = TransformWidget.X.GetComponent<CTransform>();
	XAxis.Scale(glm::vec3(0.5f));
	XAxis.Translate(glm::vec3(1.0f, -1.0f, 0.0f));
	XAxis.Rotate(glm::vec3(0.0f, 0.0f, 1.0f), -90.0f);

	auto& YAxis = TransformWidget.Y.GetComponent<CTransform>();
	YAxis.Scale(glm::vec3(0.5f));

	auto& ZAxis = TransformWidget.Z.GetComponent<CTransform>();
	ZAxis.Scale(glm::vec3(0.5f));
	ZAxis.Translate(glm::vec3(0.0f, -1.0f, 1.0f));
	ZAxis.Rotate(glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);

	auto TransformGizmo = GAssetManager.GetStaticMesh("Transform-Gizmo");

	TransformWidget.X.AddComponent<CStaticMesh>(TransformGizmo);
	TransformWidget.Y.AddComponent<CStaticMesh>(TransformGizmo);
	TransformWidget.Z.AddComponent<CStaticMesh>(TransformGizmo);

	auto& MatX = TransformWidget.X.AddComponent<CMaterial>();
	MatX.Diffuse = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

	auto& MatY = TransformWidget.Y.AddComponent<CMaterial>();
	MatY.Diffuse = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	auto& MatZ = TransformWidget.Z.AddComponent<CMaterial>();
	MatZ.Diffuse = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

	TransformWidget.X.GetComponent<CRenderer>().bVisible = false;
	TransformWidget.Y.GetComponent<CRenderer>().bVisible = false;
	TransformWidget.Z.GetComponent<CRenderer>().bVisible = false;
}

void TransformGizmoSystem::Update()
{
	auto& Scene = Scene::Get();

	if (bFirstPress && Input::GetKeyDown(Input::MouseLeft))
	{
		bFirstPress = false;
		LastX = Scene.View.LastXPos;
		LastY = Scene.View.LastYPos;
	}
	else if (Input::GetKeyUp(Input::MouseLeft))
	{
		bFirstPress = true;

		if (!(LastX == Scene.View.LastXPos && LastY == Scene.View.LastYPos))
		{
			// Don't select while looking around
			return;
		}

		Ray Ray = Scene.View.ScreenPointToRay(GPlatform->GetMousePosition());

		std::vector<Entity> ClickedEntities;

		for (auto Entity : GEntityManager.GetEntities<CStaticMesh>())
		{
			if (Entity.GetEntityID() == TransformWidget.X.GetEntityID() ||
				Entity.GetEntityID() == TransformWidget.Y.GetEntityID() ||
				Entity.GetEntityID() == TransformWidget.Z.GetEntityID())
				continue;

			if (Physics::Raycast(Ray, Entity))
			{
				ClickedEntities.push_back(Entity);
			}
		}

		if (!ClickedEntities.empty())
		{
			for (auto& Entity : ClickedEntities)
			{
				SelectedEntity = Entity;
				break;
			}
		}
		else
		{
			TransformWidget.X.GetComponent<CRenderer>().bVisible = false;
			TransformWidget.Y.GetComponent<CRenderer>().bVisible = false;
			TransformWidget.Z.GetComponent<CRenderer>().bVisible = false;

			SelectedEntity = Entity::InvalidID;
		}
	}

	if (SelectedEntity != Entity::InvalidID)
	{
		DrawOutline(SelectedEntity);
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

	TransformWidget.X.GetComponent<CRenderer>().bVisible = true;
	TransformWidget.Y.GetComponent<CRenderer>().bVisible = true;
	TransformWidget.Z.GetComponent<CRenderer>().bVisible = true;

	TransformWidget.X.GetComponent<CTransform>().SetParent(&Transform);
	TransformWidget.Y.GetComponent<CTransform>().SetParent(&Transform);
	TransformWidget.Z.GetComponent<CTransform>().SetParent(&Transform);
}