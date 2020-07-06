#pragma once
#include <ECS/System.h>
#include <ECS/Component.h>
#include <filesystem>

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

class Engine;

/** 
  * The scene system loads the initial scene from the .ini file and 
  * handles requests to load new scene files.
  */
class SceneSystem : public ISystem
{
	SYSTEM(SceneSystem);
public:
	virtual void Start(Engine& engine) override;
	virtual void Update(Engine& engine) override;

private:
	void HandleSceneLoadRequest(Engine& engine, const SceneLoadRequest& sceneLoadRequest);
};