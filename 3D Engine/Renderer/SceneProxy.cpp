#include "SceneProxy.h"
#include <Components/Light.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Engine/Engine.h>
#include <Engine/Material.h>
#include "SceneRenderer.h"
#include "ShadowProxy.h"
#include "GlobalRenderData.h"

SceneProxy::SceneProxy(Engine& Engine)
	: ECS(Engine.ECS)
{
	InitView(Engine);
	InitLights(Engine);
	InitMeshDrawCommands(Engine);

	GlobalRenderData& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();
	GlobalData.CameraDescriptorSet.Update(Engine.Device);

	GlobalData.SkyboxDescriptorSet.Skybox = drm::DescriptorImageInfo(*Engine.Scene.Skybox, Engine.Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear }));
	GlobalData.SkyboxDescriptorSet.Update(Engine.Device);
}

void SceneProxy::InitView(Engine& Engine)
{
	UNIFORM_STRUCT(CameraUniformBuffer,
		glm::mat4 WorldToView;
		glm::mat4 ViewToClip;
		glm::mat4 WorldToClip;
		glm::mat4 ClipToWorld;
		glm::vec3 Position;
		float _Pad0;
		float AspectRatio;
		float FOV;
		glm::vec2 ScreenDims;
	);

	const Camera& Camera = Engine.Camera;

	const CameraUniformBuffer CameraUniformBuffer =
	{
		Camera.GetWorldToView(),
		Camera.GetViewToClip(),
		Camera.GetWorldToClip(),
		glm::inverse(Camera.GetWorldToClip()),
		Camera.GetPosition(),
		0.0f,
		Camera.GetAspectRatio(),
		Camera.GetFOV(),
		glm::vec2(Camera.GetWidth(), Camera.GetHeight())
	};

	CameraUniform = Engine.Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(CameraUniformBuffer), &CameraUniformBuffer);
	ECS.GetSingletonComponent<GlobalRenderData>().CameraDescriptorSet.CameraUniform = CameraUniform;
}

void SceneProxy::InitLights(Engine& Engine)
{
	InitDirectionalLights(Engine);
	InitPointLights(Engine);
}

void SceneProxy::InitDirectionalLights(Engine& Engine)
{
	UNIFORM_STRUCT(DirectionalLightProxy,
		glm::vec3 Color;
		float Intensity;
		glm::vec3 Direction;
		float _Pad1;
	);

	std::vector<DirectionalLightProxy> DirectionalLightProxies;

	for (auto Entity : ECS.GetEntities<DirectionalLight>())
	{
		auto& DirectionalLight = ECS.GetComponent<struct DirectionalLight>(Entity);

		DirectionalLightProxy DirectionalLightProxy;
		DirectionalLightProxy.Intensity = DirectionalLight.Intensity;
		DirectionalLightProxy.Color = DirectionalLight.Color;
		DirectionalLightProxy.Direction = glm::normalize(DirectionalLight.Direction);

		DirectionalLightProxies.emplace_back(DirectionalLightProxy);
	}

	glm::uvec4 NumDirectionalLights;
	NumDirectionalLights.x = (uint32)DirectionalLightProxies.size();

	DirectionalLightBuffer = Engine.Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::HostVisible, sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	ECS.GetSingletonComponent<GlobalRenderData>().CameraDescriptorSet.DirectionalLightBuffer = DirectionalLightBuffer;
	void* Data = DirectionalLightBuffer.GetData();
	Platform::Memcpy(Data, &NumDirectionalLights.x, sizeof(NumDirectionalLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumDirectionalLights), DirectionalLightProxies.data(), sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
}

void SceneProxy::InitPointLights(Engine& Engine)
{
	UNIFORM_STRUCT(PointLightProxy,
		glm::vec3 Position;
		float Intensity;
		glm::vec3 Color;
		float Range;
	);

	std::vector<PointLightProxy> PointLightProxies;

	for (auto Entity : ECS.GetEntities<CPointLight>())
	{
		auto& PointLight = ECS.GetComponent<CPointLight>(Entity);
		auto& Transform = ECS.GetComponent<class Transform>(Entity);
		PointLightProxies.emplace_back(PointLightProxy{
			Transform.GetPosition(),
			PointLight.Intensity,
			PointLight.Color,
			PointLight.Range });
	}

	glm::uvec4 NumPointLights;
	NumPointLights.x = (uint32)PointLightProxies.size();

	PointLightBuffer = Engine.Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::HostVisible, sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size());
	ECS.GetSingletonComponent<GlobalRenderData>().CameraDescriptorSet.PointLightBuffer = PointLightBuffer;

	void* Data = PointLightBuffer.GetData();
	Platform::Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
}

void SceneProxy::InitMeshDrawCommands(Engine& Engine)
{
	const FrustumPlanes ViewFrustumPlanes = Engine.Camera.GetFrustumPlanes();
	
	for (auto Entity : ECS.GetEntities<MeshProxy>())
	{
		const MeshProxy& MeshProxy = ECS.GetComponent<class MeshProxy>(Entity);

		if (Physics::IsBoxInsideFrustum(ViewFrustumPlanes, MeshProxy.WorldSpaceBB))
		{
			AddToDepthPrepass(Engine.Device, Engine.ShaderMap, Engine.Assets.GetBindlessSampledImages(), MeshProxy);
			AddToLightingPass(Engine.Device, Engine.ShaderMap, Engine.Assets.GetBindlessSampledImages(), MeshProxy);
		}

		AddToVoxelsPass(Engine.Device, Engine.ShaderMap, Engine.Assets.GetBindlessSampledImages(), MeshProxy);
	}
}