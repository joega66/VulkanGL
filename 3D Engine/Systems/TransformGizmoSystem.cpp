#include "TransformGizmoSystem.h"
#include <Engine/AssetManager.h>
#include <Components/CRenderer.h>

void TransformGizmoSystem::Start()
{
	State = std::bind(&TransformGizmoSystem::Null, this, std::placeholders::_1);

	TranslateAxis =
	{
		GEntityManager.CreateEntity(),
		GEntityManager.CreateEntity(),
		GEntityManager.CreateEntity()
	};

	auto& XAxis = GEntityManager.GetComponent<CTransform>(TranslateAxis.X);
	XAxis.Scale(glm::vec3(0.5f));
	XAxis.Translate(glm::vec3(1.0f, -1.0f, 0.0f));
	XAxis.Rotate(glm::vec3(0.0f, 0.0f, 1.0f), -90.0f);

	auto& YAxis = GEntityManager.GetComponent<CTransform>(TranslateAxis.Y);
	YAxis.Scale(glm::vec3(0.5f));

	auto& ZAxis = GEntityManager.GetComponent<CTransform>(TranslateAxis.Z);
	ZAxis.Scale(glm::vec3(0.5f));
	ZAxis.Translate(glm::vec3(0.0f, -1.0f, 1.0f));
	ZAxis.Rotate(glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);

	auto TransformGizmo = GAssetManager.GetStaticMesh("TransformGizmo");

	GEntityManager.AddComponent<CStaticMesh>(TranslateAxis.X, TransformGizmo);
	GEntityManager.AddComponent<CStaticMesh>(TranslateAxis.Y, TransformGizmo);
	GEntityManager.AddComponent<CStaticMesh>(TranslateAxis.Z, TransformGizmo);
	
	auto& MatX = GEntityManager.AddComponent<CMaterial>(TranslateAxis.X);
	MatX.Diffuse = CMaterial::Green;

	auto& MatY = GEntityManager.AddComponent<CMaterial>(TranslateAxis.Y);
	MatY.Diffuse = CMaterial::Blue;

	auto& MatZ = GEntityManager.AddComponent<CMaterial>(TranslateAxis.Z);
	MatZ.Diffuse = CMaterial::Red;

	GEntityManager.GetComponent<CRenderer>(TranslateAxis.X).bVisible = false;
	GEntityManager.GetComponent<CRenderer>(TranslateAxis.Y).bVisible = false;
	GEntityManager.GetComponent<CRenderer>(TranslateAxis.Z).bVisible = false;
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
				CTransform& TransformA = GEntityManager.GetComponent<CTransform>(A);
				CTransform& TransformB = GEntityManager.GetComponent<CTransform>(B);
				return glm::distance(TransformA.GetPosition(), Scene.View.GetPosition()) <
					glm::distance(TransformB.GetPosition(), Scene.View.GetPosition());
			});

			SelectedEntity = Hits.front();

			State = std::bind(&TransformGizmoSystem::TranslateTool, this, std::placeholders::_1);
		}
		else
		{
			GEntityManager.GetComponent<CRenderer>(TranslateAxis.X).bVisible = false;
			GEntityManager.GetComponent<CRenderer>(TranslateAxis.Y).bVisible = false;
			GEntityManager.GetComponent<CRenderer>(TranslateAxis.Z).bVisible = false;

			SelectedEntity = Entity();

			State = std::bind(&TransformGizmoSystem::Null, this, std::placeholders::_1);
		}
	}
}

void TransformGizmoSystem::TranslateTool(Scene& Scene)
{
	if (Input.GetKeyDown(EKeyCode::MouseLeft))
	{
		// Check if we're grabbing an axis.
		std::vector<Entity> Gizmo = { TranslateAxis.X, TranslateAxis.Y, TranslateAxis.Z };
		Entity ClosestHit;
		float Dist = std::numeric_limits<float>::max();

		std::for_each(Gizmo.begin(), Gizmo.end(), [&](auto& Entity)
		{
			if (Physics::Raycast(Scene.View.ScreenPointToRay(Cursor.Position), Entity))
			{
				CTransform& Transform = GEntityManager.GetComponent<CTransform>(Entity);
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
	auto& StaticMesh = GEntityManager.GetComponent<CStaticMesh>(Entity);
	auto& Transform = GEntityManager.GetComponent<CTransform>(Entity);

	auto ScaledUpTransform = Transform;
	ScaledUpTransform.Scale(ScaledUpTransform.GetScale() * glm::vec3(1.05f));

	for (auto& Element : StaticMesh.StaticMesh->Batch.Elements)
	{
		Scene.Stencil.Add(Entity, DepthPassDrawPlan(Element, Transform.LocalToWorldUniform));
		Scene.Outline.Add(Entity, OutlineDrawPlan(Element, ScaledUpTransform.LocalToWorldUniform));
	}

	GEntityManager.GetComponent<CRenderer>(TranslateAxis.X).bVisible = true;
	GEntityManager.GetComponent<CRenderer>(TranslateAxis.Y).bVisible = true;
	GEntityManager.GetComponent<CRenderer>(TranslateAxis.Z).bVisible = true;

	GEntityManager.GetComponent<CTransform>(TranslateAxis.X).SetParent(&Transform);
	GEntityManager.GetComponent<CTransform>(TranslateAxis.Y).SetParent(&Transform);
	GEntityManager.GetComponent<CTransform>(TranslateAxis.Z).SetParent(&Transform);
}