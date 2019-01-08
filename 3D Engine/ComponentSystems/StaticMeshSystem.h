#pragma once
#include "ComponentSystem.h"

class StaticMeshSystem : public ComponentSystem
{
public:
	StaticMeshSystem();
	virtual void Update() override;
};