#pragma once
#include <ECS/System.h>
#include <DRM.h>

class Engine;

class UserInterface : public IRenderSystem
{
public:
	~UserInterface();
	
	virtual void Start(Engine& Engine) override;

	virtual void Update(Engine& Engine) override;

	void ShowUI(Engine& Engine);

	void ShowRenderSettings(Engine& Engine);

	/** Called in the scene renderer to display the UI. */
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