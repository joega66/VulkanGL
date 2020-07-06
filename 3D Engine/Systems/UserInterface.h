#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <GPU/GPU.h>

class Engine;
class CameraProxy;

struct ImGuiRenderData : public Component
{
	gpu::Image fontImage;
	gpu::SamplerID samplerID;

	glm::vec4 scaleAndTranslation;

	gpu::Buffer vertexBuffer;
	gpu::Buffer indexBuffer;

	PipelineStateDesc psoDesc = {};

	ImGuiRenderData(Engine& engine);

	void Render(gpu::Device& device, gpu::CommandList& cmdList, const gpu::RenderPass& renderPass);

	void Update(gpu::Device& device);
};

class UserInterface : public IRenderSystem
{
public:
	~UserInterface();
	
	virtual void Start(Engine& engine) override;
	virtual void Update(Engine& engine) override;

private:
	void ShowUI(Engine& engine);
	void ShowMainMenu(Engine& engine);
	void ShowRenderSettings(Engine& engine);
	void ShowEntities(Engine& engine);
};