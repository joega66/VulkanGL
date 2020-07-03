#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <GPU/GPU.h>

class Engine;
class CameraProxy;

struct ImGuiRenderData : public Component
{
	gpu::Image FontImage;
	gpu::SamplerID SamplerID;

	glm::vec4 ScaleAndTranslation;

	gpu::Buffer VertexBuffer;
	gpu::Buffer IndexBuffer;

	PipelineStateDesc psoDesc = {};

	ImGuiRenderData(Engine& Engine);

	void Render(gpu::Device& Device, gpu::CommandList& CmdList, const gpu::RenderPass& RenderPass);

	void Update(gpu::Device& Device);
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