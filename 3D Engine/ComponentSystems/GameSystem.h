#pragma once
#include "ComponentSystem.h"

class GameSystem : public ComponentSystem 
{
public:
	GameSystem();
	virtual void Update() override;
};