#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <GPU/GPU.h>
#include <Engine/Screen.h>

struct ImGuiRenderData : public Component
{
	gpu::Image fontImage;
	gpu::SamplerID samplerID;

	glm::vec4 scaleAndTranslation;

	gpu::Buffer vertexBuffer;
	gpu::Buffer indexBuffer;

	PipelineStateDesc psoDesc = {};

	ImGuiRenderData(gpu::Device& device, gpu::ShaderLibrary& shaderLibrary);

	void Render(gpu::Device& device, gpu::CommandList& cmdList, const gpu::RenderPass& renderPass);

	void Update(gpu::Device& device);
};

class UserInterface : public IRenderSystem
{
public:
	~UserInterface();
	
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;

	void ShowUI(Engine& engine);
	void ShowMainMenu(Engine& engine);
	void ShowRenderSettings(Engine& engine);
	void ShowEntities(Engine& engine);
};