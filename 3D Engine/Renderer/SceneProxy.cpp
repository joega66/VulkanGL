#include "SceneProxy.h"
#include <Engine/Scene.h>
#include <Engine/Screen.h>
#include <Components/CLight.h>
#include <Components/CMaterial.h>
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>

SceneProxy::SceneProxy(Scene& Scene)
	: Skybox(Scene.Skybox)
{
	InitView(Scene);
	InitLights(Scene);
	InitDrawLists(Scene);

	DescriptorSet = drm::CreateDescriptorSet();
	DescriptorSet->Write(ViewUniform, ShaderBinding(VIEW_BINDING));
	DescriptorSet->Write(DirectionalLightBuffer, ShaderBinding(DIRECTIONAL_LIGHT_BINDING));
	DescriptorSet->Write(PointLightBuffer, ShaderBinding(POINT_LIGHT_BINDING));
	DescriptorSet->Update();
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
	CheckStd140Layout<DirectionalLightProxy>();
	CheckStd140Layout<PointLightProxy>();

	auto& ECS = Scene.ECS;

	{
		for (auto Entity : ECS.GetEntities<CDirectionalLight>())
		{
			auto& DirectionalLight = ECS.GetComponent<CDirectionalLight>(Entity);
			auto& Renderer = ECS.GetComponent<CRenderer>(Entity);

			if (Renderer.bVisible)
			{
				DirectionalLightProxy DirectionalLightProxy;
				DirectionalLightProxy.Intensity = DirectionalLight.Intensity;
				DirectionalLightProxy.Color = DirectionalLight.Color;
				DirectionalLightProxy.Direction = glm::normalize(DirectionalLight.Direction);

				DirectionalLightProxies.emplace_back(DirectionalLightProxy);
			}
		}

		glm::uvec4 NumDirectionalLights;
		NumDirectionalLights.x = DirectionalLightProxies.size();

		DirectionalLightBuffer = drm::CreateStorageBuffer(sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size(), nullptr);
		void* Data = drm::LockBuffer(DirectionalLightBuffer);
		Platform.Memcpy(Data, &NumDirectionalLights.x, sizeof(NumDirectionalLights.x));
		Platform.Memcpy((uint8*)Data + sizeof(NumDirectionalLights), DirectionalLightProxies.data(), sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
		drm::UnlockBuffer(DirectionalLightBuffer);
	}
	
	{
		for (auto Entity : ECS.GetEntities<CPointLight>())
		{
			auto& PointLight = ECS.GetComponent<CPointLight>(Entity);
			auto& Transform = ECS.GetComponent<CTransform>(Entity);
			auto& Renderer = ECS.GetComponent<CRenderer>(Entity);

			if (Renderer.bVisible)
			{
				PointLightProxies.emplace_back(PointLightProxy{ Transform.GetPosition(), PointLight.Intensity, PointLight.Color, PointLight.Range });
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
}

void SceneProxy::InitDrawLists(Scene& Scene)
{
	InitLightingPassDrawList(Scene);
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