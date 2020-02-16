#pragma once
#include <ECS/System.h>

class GameSystem : public ISystem
{
	SYSTEM(GameSystem);
public:
	virtual void Start(class Engine& Engine) override;
	virtual void Update(class Engine& Engine) override;
};