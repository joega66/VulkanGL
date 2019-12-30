#include "SceneProxy.h"
#include <Engine/Scene.h>
#include <Engine/Screen.h>
#include <Components/CLight.h>
#include <Components/CMaterial.h>
#include <Components/CStaticMesh.h>
#include <Components/CTransform.h>
#include <Components/CRenderer.h>
#include "MeshProxy.h"
#include "ShadowDepthPass.h"

SceneProxy::SceneProxy(Scene& Scene)
	: View(Scene.View)
	, ECS(Scene.ECS)
	, Skybox(Scene.Skybox)
{
	InitView();
	InitLights();
	InitDrawLists();

	DescriptorSet = drm::CreateDescriptorSet();
	DescriptorSet->Write(ViewUniform, ShaderBinding(0));
	DescriptorSet->Write(DirectionalLightBuffer, ShaderBinding(1));
	DescriptorSet->Write(PointLightBuffer, ShaderBinding(2));
	DescriptorSet->Update();
}

void SceneProxy::InitView()
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
		glm::vec2 ScreenDims;
	};

	const ViewUniformData ViewUniformData =
	{
		WorldToView,
		ViewToClip,
		WorldToClip,
		View.GetPosition(),
		0.0f,
		gScreen.GetAspectRatio(),
		View.GetFOV(),
		glm::vec2(gScreen.GetWidth(), gScreen.GetHeight())
	};

	ViewUniform = drm::CreateBuffer(EBufferUsage::Uniform, sizeof(ViewUniformData), &ViewUniformData);
}

void SceneProxy::InitLights()
{
	{
		auto DirectionalLights = ECS.GetVisibleEntities<CDirectionalLight>();

		UNIFORM_STRUCT(DirectionalLightProxy,
			glm::vec3 Color;
			float Intensity;
			glm::vec3 Direction;
			int32 _Pad1;
		);

		std::vector<DirectionalLightProxy> DirectionalLightProxies;

		for (auto Entity : DirectionalLights)
		{
			auto& DirectionalLight = ECS.GetComponent<CDirectionalLight>(Entity);

			DirectionalLightProxy DirectionalLightProxy;
			DirectionalLightProxy.Intensity = DirectionalLight.Intensity;
			DirectionalLightProxy.Color = DirectionalLight.Color;
			DirectionalLightProxy.Direction = glm::normalize(DirectionalLight.Direction);

			DirectionalLightProxies.emplace_back(DirectionalLightProxy);
		}

		glm::uvec4 NumDirectionalLights;
		NumDirectionalLights.x = DirectionalLightProxies.size();

		DirectionalLightBuffer = drm::CreateBuffer(EBufferUsage::Storage, sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
		void* Data = drm::LockBuffer(DirectionalLightBuffer);
		Platform.Memcpy(Data, &NumDirectionalLights.x, sizeof(NumDirectionalLights.x));
		Platform.Memcpy((uint8*)Data + sizeof(NumDirectionalLights), DirectionalLightProxies.data(), sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
		drm::UnlockBuffer(DirectionalLightBuffer);

		auto ShadowProxies = ECS.GetVisibleEntities<CShadowProxy>();
		for (auto Entity : ShadowProxies)
		{
			auto& DirectionalLight = ECS.GetComponent<CDirectionalLight>(Entity);
			auto& ShadowProxy = ECS.GetComponent<CShadowProxy>(Entity);
			ShadowProxy.Update(DirectionalLight.Direction);
		}

		std::vector<Entity> LightsWithNoShadowProxies;
		std::set_difference(DirectionalLights.begin(), DirectionalLights.end(), ShadowProxies.begin(), ShadowProxies.end(), std::back_inserter(LightsWithNoShadowProxies));

		for (auto Entity : LightsWithNoShadowProxies)
		{
			auto& DirectionalLight = ECS.GetComponent<CDirectionalLight>(Entity);
			if (DirectionalLight.ShadowType != EShadowType::None)
			{
				ECS.AddComponent<CShadowProxy>(Entity, CShadowProxy(DirectionalLight));
			}
		}
	}
	
	{
		UNIFORM_STRUCT(PointLightProxy,
			glm::vec3 Position;
			float Intensity;
			glm::vec3 Color;
			float Range;
		);

		std::vector<PointLightProxy> PointLightProxies;

		for (auto Entity : ECS.GetVisibleEntities<CPointLight>())
		{
			auto& PointLight = ECS.GetComponent<CPointLight>(Entity);
			auto& Transform = ECS.GetComponent<CTransform>(Entity);
			PointLightProxies.emplace_back(PointLightProxy{
				Transform.GetPosition(), 
				PointLight.Intensity, 
				PointLight.Color, 
				PointLight.Range });
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

void SceneProxy::InitDrawLists()
{
	// Gather mesh proxies.
	for (auto Entity : ECS.GetVisibleEntities<CStaticMesh>())
	{
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
			MeshProxies.emplace_back(MeshProxy(
				Material,
				MeshElements,
				LocalToWorldUniform)
			);
		}
		else
		{
			const std::vector<CMaterial>& Materials = MeshBatch.Materials;

			for (uint32 ElementIndex = 0; ElementIndex < MeshElements.size(); ElementIndex++)
			{
				const CMaterial& Material = Materials[ElementIndex];
				const std::vector<MeshElement> MeshElement = { MeshElements[ElementIndex] };
				MeshProxies.emplace_back(MeshProxy(
					Material,
					MeshElement,
					LocalToWorldUniform)
				);
			}
		}
	}

	for (const MeshProxy& MeshProxy : MeshProxies)
	{
		AddToDrawLists(MeshProxy);
	}
}

void SceneProxy::AddToDrawLists(const MeshProxy& MeshProxy)
{
	DepthPrepass.Add(MeshProxy, class DepthPrepass(MeshProxy));

	const EStaticDrawListType::EStaticDrawListType StaticDrawListType =
		MeshProxy.Material.IsMasked() ?
		EStaticDrawListType::Masked : EStaticDrawListType::Opaque;

	LightingPass[StaticDrawListType].Add(MeshProxy, class LightingPass(MeshProxy));

	VoxelsPass.Add(MeshProxy, class VoxelizationPass(MeshProxy));

	ShadowDepthPass.Add(MeshProxy, class ShadowDepthPass(MeshProxy));
}