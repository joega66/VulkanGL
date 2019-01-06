#include "TransformGizmoSystem.h"
#include <Engine/ResourceManager.h>
#include <Renderer/Scene.h>

TransformGizmoSystem::TransformGizmoSystem()
{
}

void TransformGizmoSystem::RemoveOutline(Entity Entity)
{
	auto& Scene = Scene::Get();
	Scene.Stencil.Remove(Entity);
	Scene.Outline.Remove(Entity);
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

		Ray Ray = Scene.View.ScreenPointToRay();

		for (auto Entity : GEntityManager.GetEntities<CStaticMesh>())
		{
			if (Physics::Raycast(Ray, Entity))
			{
				if (Entity != SelectedEntity)
				{
					RemoveOutline(SelectedEntity);
					SelectedEntity = Entity;

					auto& StaticMesh = Entity.GetComponent<CStaticMesh>().StaticMesh;
					auto& Transform = Entity.GetComponent<CTransform>();

					auto ScaledUpTransform = Transform;
					ScaledUpTransform.Scale(glm::vec3(1.05f));

					for (auto& Resource : StaticMesh->Resources)
					{
						Scene.Stencil.Add(Entity, DepthPassDrawingPlan(Resource, Transform.LocalToWorldUniform));
						Scene.Outline.Add(Entity, OutlineDrawingPlan(Resource, ScaledUpTransform.LocalToWorldUniform));
					}
				}
				break;
			}
			else
			{
				RemoveOutline(SelectedEntity);
				SelectedEntity = Entity::InvalidID;
			}
		}
	}
}