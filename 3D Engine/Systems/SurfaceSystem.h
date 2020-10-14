#pragma once
#include <ECS/System.h>
#include <GPU/GPU.h>

class SurfaceSystem : public IRenderSystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	gpu::DescriptorSet _SurfaceSet;
	gpu::Buffer _SurfaceBuffer;
};