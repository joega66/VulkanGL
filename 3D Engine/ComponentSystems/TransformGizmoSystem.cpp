#include "TransformGizmoSystem.h"
#include "Engine/ResourceManager.h"
#include "Renderer/Scene.h"

TransformGizmoSystem::TransformGizmoSystem()
{
	// @todo Change GetComponent to std::optional<std::reference_wrapper
	//TranslateGizmo.AddComponent<CStaticMesh>(GAssetManager.GetStaticMesh("Cube"));
	// @todo This is a good place to have prefabs. A prefab is basically an entity whose
	// components aren't enabled.
	Entity TranslateGizmoPrefab = GEntityManager.CreatePrefab("Translate Gizmo");
	TranslateGizmoPrefab.AddComponent<CStaticMesh>(GAssetManager.GetStaticMesh("Cube"));

	auto E1 = GEntityManager.CreateFromPrefab("Translate Gizmo");
	auto E2 = GEntityManager.CreateFromPrefab("Translate Gizmo");
	auto E3 = GEntityManager.CreateFromPrefab("Translate Gizmo");
	auto E4 = GEntityManager.CreateFromPrefab("Translate Gizmo");

	auto& Transform = E1.GetComponent<CTransform>();
	Transform.Translate(glm::vec3(-10.0f, 0.0f, 0.0f));

	auto& Transform1 = E2.GetComponent<CTransform>();
	Transform1.Translate(glm::vec3(10.0f, 0.0f, 0.0f));

	auto& Transform2 = E3.GetComponent<CTransform>();
	Transform2.Translate(glm::vec3(0.0f, 0.0f, -10.0f));

	auto& Transform3 = E4.GetComponent<CTransform>();
	Transform3.Translate(glm::vec3(0.0f, 0.0f, 10.0f));

	// @todo Let's draw the hitboxes to see where they are landing...
}

void TransformGizmoSystem::RemoveOutline(Scene& Scene, Entity Entity)
{
	Scene.Stencil.Remove(Entity);
	Scene.Outline.Remove(Entity);
}

void TransformGizmoSystem::Update(Scene& Scene)
{
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
			// Don't cast ray while looking around
			return;
		}

		Ray Ray = Scene.View.ScreenPointToRay();

		for (auto Entity : GEntityManager.GetEntities<CStaticMesh>())
		{
			if (Physics::Raycast(Ray, Entity))
			{
				if (Entity != SelectedEntity)
				{
					RemoveOutline(Scene, SelectedEntity);

					SelectedEntity = Entity;

					auto& StaticMesh = Entity.GetComponent<CStaticMesh>().StaticMesh;
					auto& MeshTransform = Entity.GetComponent<CTransform>();

					CTransform ScaledUpTransform = MeshTransform;
					ScaledUpTransform.Scale(glm::vec3(1.05f));

					for (auto& Resource : StaticMesh->Resources)
					{
						Scene.Stencil.Add(Entity, DepthPassDrawingPlan(Resource, MeshTransform.LocalToWorldUniform));
						Scene.Outline.Add(Entity, OutlineDrawingPlan(Resource, ScaledUpTransform.LocalToWorldUniform));
					}
				}

				break;
			}
			else
			{
				RemoveOutline(Scene, SelectedEntity);
				SelectedEntity = Entity::InvalidID;
			}
		}
	}
}