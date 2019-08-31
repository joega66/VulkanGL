#pragma once
#include "System.h"

class LightSystem : public ISystem
{
	SYSTEM(LightSystem);
public:
	virtual void Update() final;
};