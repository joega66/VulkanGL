#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <DRM.h>

class Engine;
class CameraProxy;

struct ImGuiRenderData : public Component
{
	drm::Image FontImage;
	drm::SamplerID SamplerID;
	
	drm::Buffer ImguiUniform;

	drm::DescriptorSet DescriptorSet;
	drm::DescriptorSetLayout DescriptorSetLayout;

	drm::Buffer VertexBuffer;
	drm::Buffer IndexBuffer;

	PipelineStateDesc PSODesc = {};

	ImGuiRenderData(Engine& Engine);

	void Render(DRMDevice& Device, drm::CommandList& CmdList, CameraProxy& Camera);

	void Update(DRMDevice& Device);
};

class UserInterface : public IRenderSystem
{
public:
	~UserInterface();
	
	virtual void Start(Engine& Engine) override;
	virtual void Update(Engine& Engine) override;

private:
	void ShowUI(Engine& Engine);
	void ShowMainMenu(Engine& Engine);
	void ShowRenderSettings(Engine& Engine);
	void ShowEntities(Engine& Engine);
};