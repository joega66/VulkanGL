#include "SceneProxy.h"
#include <Engine/Scene.h>
#include <Engine/Screen.h>
#include <Components/CLight.h>
#include <Components/CMaterial.h>
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include "MeshProxy.h"

SceneProxy::SceneProxy(Scene& Scene)
	: View(Scene.View)
	, Skybox(Scene.Skybox)
{
	InitView(Scene);
	InitLights(Scene);
	InitDrawLists(Scene);

	DescriptorSet = drm::CreateDescriptorSet();
	DescriptorSet->Write(ViewUniform, ShaderBinding(0));
	DescriptorSet->Write(DirectionalLightBuffer, ShaderBinding(1));
	DescriptorSet->Write(PointLightBuffer, ShaderBinding(2));
	DescriptorSet->Update();
}

void SceneProxy::InitView(Scene& Scene)
{
	// Initialize view uniform.

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
		gScreen.GetAspectRatio(),
		View.GetFOV()
	};

	ViewUniform = drm::CreateBuffer(EBufferUsage::Uniform, sizeof(ViewUniformData), &ViewUniformData);
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

		DirectionalLightBuffer = drm::CreateBuffer(EBufferUsage::Storage, sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
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

		PointLightBuffer = drm::CreateBuffer(EBufferUsage::Storage, sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size());
		void* Data = drm::LockBuffer(PointLightBuffer);
		Platform.Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
		Platform.Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
		drm::UnlockBuffer(PointLightBuffer);
	}
}

void SceneProxy::InitDrawLists(Scene& Scene)
{
	auto& ECS = Scene.ECS;

	for (auto Entity : ECS.GetEntities<CStaticMesh>())
	{
		if (!ECS.GetComponent<CRenderer>(Entity).bVisible)
			continue;

		auto& Transform = ECS.GetComponent<CTransform>(Entity);

		struct LocalToWorldUniformData
		{
			glm::mat4 Transform;
			glm::mat4 Inverse;
		} LocalToWorldUniformData = { Transform.GetLocalToWorld(), glm::inverse(Transform.GetLocalToWorld()) };

		drm::BufferRef LocalToWorldUniform = drm::CreateBuffer(EBufferUsage::Uniform, sizeof(LocalToWorldUniformData), &LocalToWorldUniformData);
	
		auto& StaticMesh = ECS.GetComponent<CStaticMesh>(Entity);
		const auto& MeshBatch = StaticMesh.StaticMesh->Batch;
		const auto& MeshElements = MeshBatch.Elements;

		if (ECS.HasComponent<CMaterial>(Entity))
		{
			const CMaterial& Material = ECS.GetComponent<CMaterial>(Entity);

			MeshProxyRef MeshProxy = MakeRef<class MeshProxy>(
				Material, 
				MeshElements,
				LocalToWorldUniform
			);

			AddToDrawLists(*this, MeshProxy);
		}
		else
		{
			const std::vector<CMaterial>& Materials = MeshBatch.Materials;

			for (uint32 ElementIndex = 0; ElementIndex < MeshElements.size(); ElementIndex++)
			{
				const CMaterial& Material = Materials[ElementIndex];

				const std::vector<MeshElement> MeshElement = { MeshElements[ElementIndex] };

				MeshProxyRef MeshProxy = MakeRef<class MeshProxy>(
					Material,
					MeshElement,
					LocalToWorldUniform
				);

				AddToDrawLists(*this, MeshProxy);
			}
		}
	}
}

void SceneProxy::AddToDrawLists(SceneProxy& Scene, const MeshProxyRef& MeshProxy)
{
	const EStaticDrawListType::EStaticDrawListType StaticDrawListType =
		MeshProxy->Material.IsMasked() ?
		EStaticDrawListType::Masked : EStaticDrawListType::Opaque;

	Scene.LightingPass[StaticDrawListType].Add(MeshProxy, class LightingPass(*MeshProxy));

	Scene.VoxelsPass.Add(MeshProxy, VoxelizationPass(*MeshProxy));
}