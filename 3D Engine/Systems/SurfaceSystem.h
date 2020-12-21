#pragma once
#include <ECS/System.h>
#include <GPU/GPU.h>

BEGIN_UNIFORM_BUFFER(LocalToWorldUniform)
	MEMBER(glm::mat4, transform)
	MEMBER(glm::mat4, inverse)
	MEMBER(glm::mat4, inverseTranspose)
END_UNIFORM_BUFFER(LocalToWorldUniform)

BEGIN_DESCRIPTOR_SET(StaticMeshDescriptors)
	DESCRIPTOR(gpu::StorageBuffer, _LocalToWorldBuffer)
END_DESCRIPTOR_SET(StaticMeshDescriptors)

class SurfaceSystem : public ISystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	gpu::Buffer _SurfaceBuffer;
};