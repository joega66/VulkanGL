#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <Engine/Screen.h>
#include <filesystem>

struct SceneLoadRequest : public Component
{
	SceneLoadRequest(const std::filesystem::path& path, bool destroyOldEntities)
		: path(path), destroyOldEntities(destroyOldEntities)
	{
	}

	/** Path to the new scene. */
	std::filesystem::path path;

	/** Whether to destroy the old entities. */
	bool destroyOldEntities = true;
};

class SceneSystem : public ISystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;
};