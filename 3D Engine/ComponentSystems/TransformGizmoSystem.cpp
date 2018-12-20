#include "TransformGizmoSystem.h"
#include "Renderer/Scene.h"

void TransformGizmoSystem::Update(Scene* Scene)
{
	if (Input::GetKeyUp(Input::MouseLeft))
	{
		Ray Ray = Scene->View.ScreenPointToRay();
		for (auto Entity : GEntityManager.GetEntities<CStaticMesh, CTransform>())
		{
			if (Physics::Raycast(Ray, Entity))
			{
				if (Entity->GetEntityID() != SelectedEntity)
				{
					SelectedEntity = Entity->GetEntityID();

					auto& MeshComponent = Entity->GetComponent<CStaticMesh>();
					auto& Transform = Entity->GetComponent<CTransform>();
					auto Mesh = MeshComponent.StaticMesh;
					auto LocalToWorldUniform = Transform.LocalToWorldUniform;

					for (auto& Resource : Mesh->Resources)
					{
						Scene->Highlighted.Prepass.Add(Entity->GetEntityID(), DepthPassDrawingPlan(Resource, LocalToWorldUniform));
						Scene->Highlighted.Final.Add(Entity->GetEntityID(), ObjectHighlightDrawingPlan(Resource, LocalToWorldUniform));
					}
				}
			}
			else
			{
				Scene->Highlighted.Prepass.Remove(SelectedEntity);
				Scene->Highlighted.Final.Remove(SelectedEntity);

				SelectedEntity = Entity::InvalidID;
			}
		}
	}
}