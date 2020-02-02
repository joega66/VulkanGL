#pragma once
#include <ECS/System.h>
#include <Components/Material.h>
#include <Renderer/ShadowProxy.h>

class StaticMeshDescriptors
{
public:
	drm::BufferRef LocalToWorldUniform;
	static const std::vector<DescriptorTemplateEntry>& GetEntries()
	{
		static const std::vector<DescriptorTemplateEntry> Entries = { { 0, 1, UniformBuffer }, };
		return Entries;
	}
};

/** The render system clones game components into their renderer versions. */
class RenderSystem : public IRenderSystem
{
public:
	RenderSystem(DRMDevice& Device);

	virtual void Start(class EntityManager& ECS, DRMDevice& Device) override;
	virtual void Update(class EntityManager& ECS, DRMDevice& Device) override;

	DescriptorTemplate<MaterialDescriptors>	MaterialTemplate;

	DescriptorTemplate<StaticMeshDescriptors> StaticMeshTemplate;

	DescriptorTemplate<ShadowDescriptors> ShadowTemplate;
};