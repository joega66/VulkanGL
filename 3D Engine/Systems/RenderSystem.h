#pragma once
#include <ECS/System.h>
#include <GPU/GPU.h>
#include <Engine/Screen.h>

class Engine;

/** The render system clones game components into their renderer versions. */
class RenderSystem : public IRenderSystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;

	gpu::DescriptorSet _SurfaceSet;
	gpu::Buffer _SurfaceBuffer;
};