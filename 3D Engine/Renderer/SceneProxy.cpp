#include "SceneProxy.h"
#include <Engine/Scene.h>
#include <Engine/Screen.h>
#include <Components/CLight.h>
#include <Components/Material.h>
#include <Components/StaticMeshComponent.h>
#include <Components/Transform.h>
#include <Components/CRenderer.h>
#include "MeshProxy.h"
#include "ShadowProxy.h"

SceneProxy::SceneProxy(DRM& Device, Scene& Scene)
	: ShaderMap(Scene.ShaderMap)
	, View(Scene.View)
	, ECS(Scene.ECS)
	, Skybox(Scene.Skybox)
{
	InitView(Device);
	InitLights(Device);
	InitMeshDrawCommands(Device);

	DescriptorSet = Device.CreateDescriptorSet();
	DescriptorSet->Write(ViewUniform, 0);
	DescriptorSet->Write(DirectionalLightBuffer, 1);
	DescriptorSet->Write(PointLightBuffer, 2);
	DescriptorSet->Update();
}

void SceneProxy::InitView(DRM& Device)
{
	UNIFORM_STRUCT(ViewUniformBuffer,
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

	const glm::mat4 WorldToView = View.GetWorldToView();
	const glm::mat4& ViewToClip = View.GetViewToClip();
	const glm::mat4 WorldToClip = View.GetWorldToClip();
	const ViewUniformBuffer ViewUniformBuffer =
	{
		WorldToView,
		ViewToClip,
		WorldToClip,
		glm::inverse(WorldToClip),
		View.GetPosition(),
		0.0f,
		gScreen.GetAspectRatio(),
		View.GetFOV(),
		glm::vec2(gScreen.GetWidth(), gScreen.GetHeight())
	};

	ViewUniform = Device.CreateBuffer(EBufferUsage::Uniform, sizeof(ViewUniformBuffer), &ViewUniformBuffer);
}

void SceneProxy::InitLights(DRM& Device)
{
	InitDirectionalLights(Device);
	InitPointLights(Device);
}

void SceneProxy::InitDirectionalLights(DRM& Device)
{
	UNIFORM_STRUCT(DirectionalLightProxy,
		glm::vec3 Color;
		float Intensity;
		glm::vec3 Direction;
		float _Pad1;
	);

	std::vector<DirectionalLightProxy> DirectionalLightProxies;

	for (auto Entity : ECS.GetVisibleEntities<CDirectionalLight>())
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

	DirectionalLightBuffer = Device.CreateBuffer(EBufferUsage::Storage, sizeof(NumDirectionalLights) + sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	void* Data = Device.LockBuffer(DirectionalLightBuffer);
	Platform.Memcpy(Data, &NumDirectionalLights.x, sizeof(NumDirectionalLights.x));
	Platform.Memcpy((uint8*)Data + sizeof(NumDirectionalLights), DirectionalLightProxies.data(), sizeof(DirectionalLightProxy) * DirectionalLightProxies.size());
	Device.UnlockBuffer(DirectionalLightBuffer);

	for (auto Entity : ECS.GetEntities<ShadowProxy>())
	{
		auto& DirectionalLight = ECS.GetComponent<CDirectionalLight>(Entity);
		auto& ShadowProxy = ECS.GetComponent<class ShadowProxy>(Entity);
		ShadowProxy.Update(Device, DirectionalLight);
	}
}

void SceneProxy::InitPointLights(DRM& Device)
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
		auto& Transform = ECS.GetComponent<class Transform>(Entity);
		PointLightProxies.emplace_back(PointLightProxy{
			Transform.GetPosition(),
			PointLight.Intensity,
			PointLight.Color,
			PointLight.Range });
	}

	glm::uvec4 NumPointLights;
	NumPointLights.x = PointLightProxies.size();

	PointLightBuffer = Device.CreateBuffer(EBufferUsage::Storage, sizeof(NumPointLights) + sizeof(PointLightProxy) * PointLightProxies.size());
	void* Data = Device.LockBuffer(PointLightBuffer);
	Platform.Memcpy(Data, &NumPointLights.x, sizeof(NumPointLights.x));
	Platform.Memcpy((uint8*)Data + sizeof(NumPointLights), PointLightProxies.data(), sizeof(PointLightProxy) * PointLightProxies.size());
	Device.UnlockBuffer(PointLightBuffer);
}

void SceneProxy::InitMeshDrawCommands(DRM& Device)
{
	auto StaticMeshEntities = ECS.GetVisibleEntities<StaticMeshComponent>();
	
	MeshProxies.reserve(StaticMeshEntities.size());
	VisibleMeshProxies.reserve(StaticMeshEntities.size());

	const FrustumPlanes ViewFrustumPlanes = View.GetFrustumPlanes();

	// Gather mesh proxies.
	for (auto Entity : StaticMeshEntities)
	{
		UNIFORM_STRUCT(LocalToWorldUniformBuffer,
			glm::mat4 Transform;
			glm::mat4 Inverse;
		);

		const Transform& Transform = ECS.GetComponent<class Transform>(Entity);
		const LocalToWorldUniformBuffer LocalToWorldUniformBuffer = { Transform.GetLocalToWorld(), glm::inverse(Transform.GetLocalToWorld()) };
		const drm::BufferRef LocalToWorldUniform = Device.CreateBuffer(EBufferUsage::Uniform, sizeof(LocalToWorldUniformBuffer), &LocalToWorldUniformBuffer);
		const StaticMesh* StaticMesh = ECS.GetComponent<StaticMeshComponent>(Entity).StaticMesh;
		const Material& Material = ECS.GetComponent<class Material>(Entity);

		drm::DescriptorSetRef MeshSet = Device.CreateDescriptorSet();
		MeshSet->Write(LocalToWorldUniform, 0);
		MeshSet->Update();

		MeshProxies.emplace_back(MeshProxy(Device, MeshSet, Material, StaticMesh->Submeshes, LocalToWorldUniform));
		
		const BoundingBox WorldSpaceBB = StaticMesh->Bounds.Transform(Transform.GetLocalToWorld());
		
		if (Physics::IsBoxInsideFrustum(ViewFrustumPlanes, WorldSpaceBB))
		{
			VisibleMeshProxies.push_back(&MeshProxies.back());
		}
	}

	// Add to scene draw lists.
	for (const MeshProxy& MeshProxy : MeshProxies)
	{
		AddToShadowDepthPass(MeshProxy);
		AddToVoxelsPass(MeshProxy);
	}

	// Add to visible draw lists.
	for (const MeshProxy* MeshProxy : VisibleMeshProxies)
	{
		AddToDepthPrepass(*MeshProxy);
		AddToLightingPass(*MeshProxy);
	}
}