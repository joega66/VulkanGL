#include "SceneProxy.h"
#include <Engine/Scene.h>
#include <Components/Light.h>
#include <Components/Material.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include "ShadowProxy.h"

SceneProxy::SceneProxy(DRMDevice& Device, Scene& Scene)
	: Camera(Scene.Camera)
	, ShaderMap(Scene.ShaderMap)
	, ECS(Scene.ECS)
	, SkyboxDescriptorSet(Device)
	, DescriptorSet(Device)
{
	InitView(Device);
	InitLights(Device);
	InitMeshDrawCommands(Device);
	
	DescriptorSet.Update();

	SkyboxDescriptorSet.Skybox = Scene.Skybox;
	SkyboxDescriptorSet.Update();
}

void SceneProxy::InitView(DRMDevice& Device)
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

	const CameraUniformBuffer CameraUniformBuffer =
	{
		GetWorldToView(),
		GetViewToClip(),
		GetWorldToClip(),
		glm::inverse(GetWorldToClip()),
		GetPosition(),
		0.0f,
		GetAspectRatio(),
		GetFOV(),
		glm::vec2(GetWidth(), GetHeight())
	};

	DescriptorSet.CameraUniform = Device.CreateBuffer(EBufferUsage::Uniform, sizeof(CameraUniformBuffer), &CameraUniformBuffer);
}

void SceneProxy::InitLights(DRMDevice& Device)
{
	InitDirectionalLights(Device);
	InitPointLights(Device);
}

void SceneProxy::InitDirectionalLights(DRMDevice& Device)
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
	NumDirectionalLights.x = DirectionalLightProxies.size();

	DescriptorSet.DirectionalLightBuffer = Device.CreateBuffer(EBufferUsage::Storage, sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	void* Data = Device.LockBuffer(DescriptorSet.DirectionalLightBuffer);
	Platform::Memcpy(Data, &NumDirectionalLights.x, sizeof(NumDirectionalLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumDirectionalLights), DirectionalLightProxies.data(), sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	Device.UnlockBuffer(DescriptorSet.DirectionalLightBuffer);
}

void SceneProxy::InitPointLights(DRMDevice& Device)
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
	NumPointLights.x = PointLightProxies.size();

	DescriptorSet.PointLightBuffer = Device.CreateBuffer(EBufferUsage::Storage, sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size());
	void* Data = Device.LockBuffer(DescriptorSet.PointLightBuffer);
	Platform::Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
	Device.UnlockBuffer(DescriptorSet.PointLightBuffer);
}

void SceneProxy::InitMeshDrawCommands(DRMDevice& Device)
{
	const FrustumPlanes ViewFrustumPlanes = GetFrustumPlanes();

	for (auto Entity : ECS.GetEntities<MeshProxy>())
	{
		const MeshProxy& MeshProxy = ECS.GetComponent<class MeshProxy>(Entity);

		if (Physics::IsBoxInsideFrustum(ViewFrustumPlanes, MeshProxy.WorldSpaceBB))
		{
			AddToDepthPrepass(MeshProxy);
			AddToLightingPass(MeshProxy);
		}

		AddToShadowDepthPass(MeshProxy);
		AddToVoxelsPass(MeshProxy);
	}
}