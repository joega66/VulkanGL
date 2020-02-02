#pragma once
#include <ECS/System.h>
#include <DRM.h>

/** The render system clones game components into their renderer versions. */
class RenderSystem : public IRenderSystem
{
	SYSTEM(RenderSystem);
public:
	virtual void Start(class EntityManager& ECS, class DRMDevice& Device) override;
	virtual void Update(class EntityManager& ECS, class DRMDevice& Device) override;

	drm::DescriptorTemplateRef MaterialTemplate;
	drm::DescriptorTemplateRef StaticMeshTemplate;
};