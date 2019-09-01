#pragma once
#include <ECS/System.h>

class StaticMeshSystem : public ISystem
{
	SYSTEM(StaticMeshSystem);
public:
	virtual void Update(class Scene& Scene) override;
};