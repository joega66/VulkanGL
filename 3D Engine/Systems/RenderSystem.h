#pragma once
#include <ECS/System.h>

/** The render system clones game components into their renderer versions. */
class RenderSystem : public IRenderSystem
{
	SYSTEM(RenderSystem);
public:
	virtual void Start(class EntityManager& ECS, class DRMDevice& Device) override;
	virtual void Update(class EntityManager& ECS, class DRMDevice& Device) override;
};