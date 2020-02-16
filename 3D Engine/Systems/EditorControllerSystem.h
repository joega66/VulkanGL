#pragma once
#include <ECS/System.h>

class EditorControllerSystem : public ISystem
{
	SYSTEM(EditorControllerSystem);
public:
	virtual void Start(class Engine& Engine) override;
	virtual void Update(class Engine& Engine) override;
};