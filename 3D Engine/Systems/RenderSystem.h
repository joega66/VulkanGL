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
	RenderSystem(class Engine& Engine);

	virtual void Start(class Engine& Engine) override;
	virtual void Update(class Engine& Engine) override;

	DescriptorTemplate<MaterialDescriptors>	MaterialTemplate;

	DescriptorTemplate<StaticMeshDescriptors> StaticMeshTemplate;

	DescriptorTemplate<ShadowDescriptors> ShadowTemplate;
};