#pragma once
#include "System.h"

class GameSystem : public ISystem 
{
	SYSTEM(GameSystem);
public:
	virtual void Start() override;
	virtual void Update(class Scene& Scene) override;
};