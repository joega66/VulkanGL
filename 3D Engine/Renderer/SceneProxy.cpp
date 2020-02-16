#include "SceneProxy.h"
#include <Components/Light.h>
#include <Components/Material.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Engine/Engine.h>
#include "ShadowProxy.h"

SceneProxy::SceneProxy(Engine& Engine)
	: Camera(Engine.Camera)
	, ECS(Engine.ECS)
	, SkyboxDescriptorSet(Engine.Device)
	, DescriptorSet(Engine.Device)
{
	InitView(Engine);
	InitLights(Engine);
	InitMeshDrawCommands(Engine);
	
	DescriptorSet.Update();

	SkyboxDescriptorSet.Skybox = Engine.Scene.Skybox;
	SkyboxDescriptorSet.Update();
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

	DescriptorSet.CameraUniform = Engine.Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::KeepCPUAccessible, sizeof(CameraUniformBuffer), &CameraUniformBuffer);
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
	NumDirectionalLights.x = DirectionalLightProxies.size();

	DescriptorSet.DirectionalLightBuffer = Engine.Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::KeepCPUAccessible, sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	void* Data = Engine.Device.LockBuffer(DescriptorSet.DirectionalLightBuffer);
	Platform::Memcpy(Data, &NumDirectionalLights.x, sizeof(NumDirectionalLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumDirectionalLights), DirectionalLightProxies.data(), sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	Engine.Device.UnlockBuffer(DescriptorSet.DirectionalLightBuffer);
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
	NumPointLights.x = PointLightProxies.size();

	DescriptorSet.PointLightBuffer = Engine.Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::KeepCPUAccessible, sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size());
	void* Data = Engine.Device.LockBuffer(DescriptorSet.PointLightBuffer);
	Platform::Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
	Engine.Device.UnlockBuffer(DescriptorSet.PointLightBuffer);
}

void SceneProxy::InitMeshDrawCommands(Engine& Engine)
{
	const FrustumPlanes ViewFrustumPlanes = GetFrustumPlanes();
	
	for (auto Entity : ECS.GetEntities<MeshProxy>())
	{
		const MeshProxy& MeshProxy = ECS.GetComponent<class MeshProxy>(Entity);

		if (Physics::IsBoxInsideFrustum(ViewFrustumPlanes, MeshProxy.WorldSpaceBB))
		{
			AddToDepthPrepass(Engine.ShaderMap, MeshProxy);
			AddToLightingPass(Engine.ShaderMap, MeshProxy);
		}

		AddToShadowDepthPass(Engine.ShaderMap, MeshProxy);
		AddToVoxelsPass(Engine.ShaderMap, MeshProxy);
	}
}