#include "SceneProxy.h"
#include <Engine/Scene.h>
#include <Engine/Screen.h>
#include <Components/CLight.h>
#include <Components/CMaterial.h>
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include <Components/COutline.h>

SceneProxy::SceneProxy(Scene& Scene)
	: Skybox(Scene.Skybox)
{
	InitView(Scene);
	InitLights(Scene);
	InitDrawLists(Scene);
}

void SceneProxy::InitView(Scene& Scene)
{
	// Initialize view uniform.
	View& View = Scene.View;

	const glm::mat4 WorldToView = View.GetWorldToView();
	const glm::mat4 ViewToClip = View.GetViewToClip();
	const glm::mat4 WorldToClip = ViewToClip * WorldToView;

	struct ViewUniformData
	{
		glm::mat4 WorldToView;
		glm::mat4 ViewToClip;
		glm::mat4 WorldToClip;
		glm::vec3 Position;
		float _Pad0;
		float AspectRatio;
		float FOV;
		glm::vec2 _Pad1;
	};

	const ViewUniformData ViewUniformData =
	{
		WorldToView,
		ViewToClip,
		WorldToClip,
		View.GetPosition(),
		0.0f,
		(float)Screen.GetWidth() / Screen.GetHeight(),
		View.GetFOV()
	};

	ViewUniform = drm::CreateUniformBuffer(sizeof(ViewUniformData), &ViewUniformData, EUniformUpdate::SingleFrame);
}

void SceneProxy::InitLights(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	for (auto Entity : ECS.GetEntities<CLight>())
	{
		auto& Light = ECS.GetComponent<CLight>(Entity);
		auto& Transform = ECS.GetComponent<CTransform>(Entity);
		auto& Renderer = ECS.GetComponent<CRenderer>(Entity);

		if (Renderer.bVisible)
		{
			PointLightProxies.emplace_back(PointLightProxy{ Transform.GetPosition(), Light.Intensity, Light.Color, Light.Range });
		}
	}

	glm::uvec4 NumPointLights;
	NumPointLights.x = PointLightProxies.size();

	PointLightBuffer = drm::CreateStorageBuffer(sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size(), nullptr);
	void* Data = drm::LockBuffer(PointLightBuffer);
	Platform.Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform.Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
	drm::UnlockBuffer(PointLightBuffer);
}

void SceneProxy::InitDrawLists(Scene& Scene)
{
	InitLightingPassDrawList(Scene);
	InitOutlineDrawList(Scene);
}

void SceneProxy::InitLightingPassDrawList(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	for (auto Entity : ECS.GetEntities<CStaticMesh>())
	{
		if (!ECS.GetComponent<CRenderer>(Entity).bVisible)
			continue;

		auto& StaticMesh = ECS.GetComponent<CStaticMesh>(Entity);
		auto& Transform = ECS.GetComponent<CTransform>(Entity);

		if (ECS.HasComponent<CMaterial>(Entity))
		{
			// The entity has a Material -- render using it.
			auto& Material = ECS.GetComponent<CMaterial>(Entity);
			auto& Batch = StaticMesh.StaticMesh->Batch;
			auto& Elements = Batch.Elements;
			for (auto& Element : Elements)
			{
				LightingPass.Add(Entity, LightingPassDrawPlan(Element, Material, Transform.LocalToWorldUniform));
			}
		}
		else
		{
			// Use the materials that came with the static mesh.
			auto& Batch = StaticMesh.StaticMesh->Batch;
			auto& Elements = Batch.Elements;
			for (auto& Element : Elements)
			{
				LightingPass.Add(Entity, LightingPassDrawPlan(Element, Element.Material, Transform.LocalToWorldUniform));
			}
		}
	}
}

void SceneProxy::InitOutlineDrawList(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	for (auto Entity : ECS.GetEntities<COutline>())
	{
		auto& Transform = ECS.GetComponent<CTransform>(Entity);
		auto& StaticMesh = ECS.GetComponent<CStaticMesh>(Entity);
		auto ScaledUpTransform = Transform;
		ScaledUpTransform.Scale(ScaledUpTransform.GetScale() * glm::vec3(1.05f));

		for (auto& Element : StaticMesh.StaticMesh->Batch.Elements)
		{
			Stencil.Add(Entity, DepthPassDrawPlan(Element, Transform.LocalToWorldUniform));
			Outline.Add(Entity, OutlineDrawPlan(Element, ScaledUpTransform.LocalToWorldUniform));
		}
	}
}