#pragma once
#include <ECS/System.h>
#include <DRM.h>

class Engine;

struct ImGuiRenderData
{
	drm::Image FontImage;
	drm::Buffer ImguiUniform;

	drm::DescriptorSet DescriptorSet;
	drm::DescriptorSetLayout DescriptorSetLayout;

	drm::Buffer VertexBuffer;
	drm::Buffer IndexBuffer;

	std::shared_ptr<drm::Pipeline> Pipeline;

	ImGuiRenderData(DRMDevice& Device);

	void Render(drm::CommandList& CmdList);

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
};