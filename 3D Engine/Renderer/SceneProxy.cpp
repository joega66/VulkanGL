#include "SceneProxy.h"
#include <Components/Light.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Engine/Engine.h>
#include <Engine/Material.h>
#include "SceneRenderer.h"
#include "ShadowProxy.h"

SceneProxy::SceneProxy(Engine& Engine, SceneRenderer& SceneRenderer)
	: ECS(Engine.ECS)
{
	InitView(Engine, SceneRenderer);
	InitLights(Engine, SceneRenderer);
	InitMeshDrawCommands(Engine, SceneRenderer);

	SceneRenderer.CameraDescriptorSet.Update();

	SceneRenderer.SkyboxDescriptorSet.Skybox = Engine.Scene.Skybox;
	SceneRenderer.SkyboxDescriptorSet.Sampler = Engine.Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::ClampToEdge, ESamplerMipmapMode::Linear });
	SceneRenderer.SkyboxDescriptorSet.Update();
}

void SceneProxy::InitView(Engine& Engine, SceneRenderer& SceneRenderer)
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
	SceneRenderer.CameraDescriptorSet.CameraUniform = &CameraUniform;
}

void SceneProxy::InitLights(Engine& Engine, SceneRenderer& SceneRenderer)
{
	InitDirectionalLights(Engine, SceneRenderer);
	InitPointLights(Engine, SceneRenderer);
}

void SceneProxy::InitDirectionalLights(Engine& Engine, SceneRenderer& SceneRenderer)
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

	DirectionalLightBuffer = Engine.Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::HostVisible, sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	SceneRenderer.CameraDescriptorSet.DirectionalLightBuffer = &DirectionalLightBuffer;
	void* Data = Engine.Device.LockBuffer(DirectionalLightBuffer);
	Platform::Memcpy(Data, &NumDirectionalLights.x, sizeof(NumDirectionalLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumDirectionalLights), DirectionalLightProxies.data(), sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	Engine.Device.UnlockBuffer(DirectionalLightBuffer);
}

void SceneProxy::InitPointLights(Engine& Engine, SceneRenderer& SceneRenderer)
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

	PointLightBuffer = Engine.Device.CreateBuffer(EBufferUsage::Storage | EBufferUsage::HostVisible, sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size());
	SceneRenderer.CameraDescriptorSet.PointLightBuffer = &PointLightBuffer;

	void* Data = Engine.Device.LockBuffer(PointLightBuffer);
	Platform::Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform::Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
	Engine.Device.UnlockBuffer(PointLightBuffer);
}

void SceneProxy::InitMeshDrawCommands(Engine& Engine, SceneRenderer& SceneRenderer)
{
	const FrustumPlanes ViewFrustumPlanes = Engine.Camera.GetFrustumPlanes();
	
	for (auto Entity : ECS.GetEntities<MeshProxy>())
	{
		const MeshProxy& MeshProxy = ECS.GetComponent<class MeshProxy>(Entity);

		if (Physics::IsBoxInsideFrustum(ViewFrustumPlanes, MeshProxy.WorldSpaceBB))
		{
			AddToDepthPrepass(SceneRenderer, Engine.Device, Engine.ShaderMap, MeshProxy);
			AddToLightingPass(SceneRenderer, Engine.Device, Engine.ShaderMap, MeshProxy);
		}

		AddToVoxelsPass(SceneRenderer, Engine.Device, Engine.ShaderMap, MeshProxy);
	}
}