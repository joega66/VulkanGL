#pragma once
#include "System.h"

class StaticMeshSystem : public ISystem
{
	SYSTEM(StaticMeshSystem);
public:
	virtual void Update() override;
};