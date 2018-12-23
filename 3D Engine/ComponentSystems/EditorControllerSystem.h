#pragma once
#include "ComponentSystem.h"

class EditorControllerSystem : public ComponentSystem
{
public:
	virtual void Update(class Scene& Scene) override;
};