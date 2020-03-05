#pragma once
#include <ECS/System.h>
#include <filesystem>

struct SceneLoadRequest
{
	/** Path to the new scene. */
	std::filesystem::path Path;
	/** Whether to destroy the old scene entities. */
	bool DestroyOldEntities = true;
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
	virtual void Start(Engine& Engine) override;
	virtual void Update(Engine& Engine) override;

private:
	void HandleSceneLoadRequest(Engine& Engine, const SceneLoadRequest& SceneLoadRequest);

};