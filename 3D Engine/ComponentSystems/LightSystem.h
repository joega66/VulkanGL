#pragma once
#include "ComponentSystem.h"

class LightSystem : public ComponentSystem
{
public:
	virtual void Update() final;
};