#pragma once
#include <ECS/System.h>
#include <DRMShader.h>
#include <DRM.h>

class UserInterface : public IRenderSystem
{
public:
	UserInterface(class Engine& Engine);

	~UserInterface();
	
	virtual void Start(class Engine& Engine) override;

	virtual void Update(class Engine& Engine) override;

	void Render(const drm::RenderPass& RenderPass, drm::CommandList& CmdList);

private:
	void CreateImGuiRenderResources(DRMDevice& Device);

	void UploadImGuiDrawData(DRMDevice& Device);

	drm::Image FontImage;
	drm::BufferRef ImguiUniform;

	drm::DescriptorSetRef DescriptorSet;
	drm::DescriptorTemplateRef DescriptorTemplate;

	drm::BufferRef VertexBuffer;
	drm::BufferRef IndexBuffer;

	PipelineStateDesc PSODesc;
};