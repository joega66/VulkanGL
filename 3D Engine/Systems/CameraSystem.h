#pragma once
#include <ECS/System.h>
#include <Engine/Screen.h>

class CameraSystem : public IRenderSystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;
};