#pragma once
#include <ECS/System.h>

class GameSystem : public ISystem
{
	SYSTEM(GameSystem);
public:
	virtual void Start(class Scene& Scene) override;
	virtual void Update(class Scene& Scene) override;
};