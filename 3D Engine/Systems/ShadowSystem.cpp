#include "ShadowSystem.h"
#include <Components/Transform.h>
#include <Components/Light.h>
#include <Renderer/ShadowRender.h>
#include <Engine/Engine.h>

DECLARE_UNIFORM_BUFFER(ShadowUniform)

DECLARE_DESCRIPTOR_SET(ShadowDescriptors);

void ShadowSystem::Start(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;

	ecs.OnComponentCreated<DirectionalLight>([&] (Entity& entity, DirectionalLight& directionalLight)
	{
		ecs.AddComponent(entity, ShadowRender(device, directionalLight));
	});
}

void ShadowSystem::Update(Engine& engine)
{
	auto& ecs = engine._ECS;
	auto& device = engine._Device;

	auto entities = ecs.GetEntities<ShadowRender>();

	_ShadowUniform = device.CreateBuffer(EBufferUsage::Uniform, EMemoryUsage::CPU_TO_GPU, entities.size() * sizeof(ShadowUniform));

	ShadowDescriptors descriptors;
	descriptors._ShadowUniform = _ShadowUniform;

	device.UpdateDescriptorSet(descriptors);

	auto shadowUniformData = static_cast<ShadowUniform*>(_ShadowUniform.GetData());

	int i = 0;

	for (auto entity : entities)
	{
		const auto& directionalLight = ecs.GetComponent<DirectionalLight>(entity);
		const auto& transform = ecs.GetComponent<Transform>(entity);
		auto& shadowRender = ecs.GetComponent<ShadowRender>(entity);

		shadowRender.Update(device, directionalLight, transform, i * sizeof(ShadowUniform));

		shadowUniformData[i].lightViewProj = shadowRender.GetLightViewProjMatrix();
		shadowUniformData[i].invLightViewProj = shadowRender.GetLightViewProjMatrixInv();

		i++;
	}
}