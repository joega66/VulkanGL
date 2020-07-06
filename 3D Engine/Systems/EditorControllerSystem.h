#pragma once
#include <ECS/System.h>

class Engine;

class EditorControllerSystem : public ISystem
{
	SYSTEM(EditorControllerSystem);
public:
	virtual void Start(Engine& engine) override;
	virtual void Update(Engine& engine) override;
};