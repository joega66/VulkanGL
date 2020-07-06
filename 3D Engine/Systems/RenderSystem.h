#pragma once
#include <ECS/System.h>

class Engine;

/** The render system clones game components into their renderer versions. */
class RenderSystem : public IRenderSystem
{
public:
	virtual void Start(Engine& engine) override;
	virtual void Update(Engine& engine) override;
};