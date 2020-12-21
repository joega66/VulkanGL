#pragma once
#include <ECS/System.h>
#include <GPU/GPU.h>

BEGIN_UNIFORM_BUFFER(ShadowUniform)
	MEMBER(glm::mat4, lightViewProj)
	MEMBER(glm::mat4, invLightViewProj)
END_UNIFORM_BUFFER(ShadowUniform)

BEGIN_DESCRIPTOR_SET(ShadowDescriptors)
	DESCRIPTOR(gpu::UniformBuffer<ShadowUniform>, _ShadowUniform)
END_DESCRIPTOR_SET(ShadowDescriptors)

class ShadowSystem : public ISystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	gpu::Buffer _ShadowUniform;
};