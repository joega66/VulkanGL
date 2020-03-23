#pragma once
#include <ECS/System.h>
#include <Engine/Material.h>
#include <Renderer/ShadowProxy.h>

class StaticMeshDescriptors
{
public:
	drm::DescriptorBufferInfo LocalToWorldUniform;
	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings = { { 0, 1, UniformBuffer }, };
		return Bindings;
	}
};

/** The render system clones game components into their renderer versions. */
class RenderSystem : public IRenderSystem
{
public:
	RenderSystem(class Engine& Engine);

	virtual void Start(class Engine& Engine) override;
	virtual void Update(class Engine& Engine) override;

	DescriptorSetLayout<StaticMeshDescriptors> StaticMeshLayout;

	DescriptorSetLayout<ShadowDescriptors> ShadowLayout;
};