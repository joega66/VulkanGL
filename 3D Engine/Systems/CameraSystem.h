#pragma once
#include <ECS/System.h>
#include <Engine/Screen.h>
#include <GPU/GPU.h>

BEGIN_UNIFORM_BUFFER(CameraUniform)
	MEMBER(glm::mat4, worldToView)
	MEMBER(glm::mat4, viewToClip)
	MEMBER(glm::mat4, worldToClip)
	MEMBER(glm::mat4, clipToWorld)
	MEMBER(glm::mat4, prevWorldToClip)
	MEMBER(glm::vec3, position)
	MEMBER(float, _pad0)
	MEMBER(float, aspectRatio)
	MEMBER(float, fov)
	MEMBER(glm::vec2, screenDims)
	MEMBER(glm::vec3, clipData)
	MEMBER(float, _pad1)
END_UNIFORM_BUFFER(CameraUniform)

BEGIN_DESCRIPTOR_SET(CameraDescriptors)
	DESCRIPTOR(gpu::UniformBuffer<CameraUniform>, _CameraUniform)
	DESCRIPTOR(gpu::SampledImage, _SceneDepth)
	DESCRIPTOR(gpu::SampledImage, _GBuffer0)
	DESCRIPTOR(gpu::SampledImage, _GBuffer1)
	DESCRIPTOR(gpu::StorageImage, _SceneColor)
	DESCRIPTOR(gpu::StorageImage, _SSRHistory)
	DESCRIPTOR(gpu::StorageImage, _SSGIHistory)
	DESCRIPTOR(gpu::StorageImage, _DirectLighting)
END_DESCRIPTOR_SET(CameraDescriptors)

class CameraSystem : public ISystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;

	gpu::Buffer _CameraUniform;
};