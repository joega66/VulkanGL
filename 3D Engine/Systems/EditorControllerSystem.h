#pragma once
#include <ECS/System.h>

class EditorControllerSystem : public ISystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;
};