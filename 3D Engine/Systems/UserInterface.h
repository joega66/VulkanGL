#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <GPU/GPU.h>
#include <Engine/Screen.h>

struct UserInterfaceRender : public Component
{
	gpu::Image fontImage;
	gpu::TextureID fontTexture;

	glm::vec4 scaleAndTranslation;

	gpu::Buffer vertexBuffer;
	gpu::Buffer indexBuffer;

	gpu::Pipeline pipeline;

	UserInterfaceRender(gpu::Device& device);

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
	std::list<gpu::TextureID> _UserTextures;

	void ShowUI(Engine& engine);
	void ShowMainMenu(Engine& engine);
	void ShowRenderSettings(Engine& engine);
	void ShowEntities(Engine& engine);
};