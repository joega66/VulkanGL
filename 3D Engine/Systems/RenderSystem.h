#pragma once
#include <ECS/System.h>
#include <GPU/GPU.h>

class Engine;

/** The render system clones game components into their renderer versions. */
class RenderSystem : public IRenderSystem
{
public:
	virtual void Start(Engine& engine) override;
	virtual void Update(Engine& engine) override;

	gpu::DescriptorSet _SurfaceSet;
	gpu::Buffer _SurfaceBuffer;
};