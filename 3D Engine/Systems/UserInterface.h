#pragma once
#include <ECS/System.h>
#include <DRM.h>

class UserInterface : public IRenderSystem
{
public:
	UserInterface(class Engine& Engine);

	~UserInterface();
	
	virtual void Start(class Engine& Engine) override;

	virtual void Update(class Engine& Engine) override;

	void Render(DRMDevice& Device, const drm::RenderPass& RenderPass, drm::CommandList& CmdList);

private:
	void CreateImGuiRenderResources(DRMDevice& Device);

	void UploadImGuiDrawData(DRMDevice& Device);

	drm::Image FontImage;
	drm::Buffer ImguiUniform;

	drm::DescriptorSet DescriptorSet;
	drm::DescriptorSetLayout DescriptorSetLayout;

	drm::Buffer VertexBuffer;
	drm::Buffer IndexBuffer;

	PipelineStateDesc PSODesc;
};