#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <Engine/Screen.h>
#include <filesystem>

class Engine;

struct SceneLoadRequest : public Component
{
	SceneLoadRequest(const std::filesystem::path& path, bool destroyOldEntities)
		: path(path), destroyOldEntities(destroyOldEntities)
	{
	}

	/** Path to the new scene. */
	std::filesystem::path path;
	/** Whether to destroy the old scene entities. */
	bool destroyOldEntities = true;
};

/** 
  * The scene system loads the initial scene from the .ini file and 
  * handles requests to load new scene files.
  */
class SceneSystem : public ISystem
{
	SYSTEM(SceneSystem);
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;

private:
	std::shared_ptr<ScreenResizeEvent> _ScreenResizeEvent;

	void HandleSceneLoadRequest(Engine& engine, const SceneLoadRequest& sceneLoadRequest);
};