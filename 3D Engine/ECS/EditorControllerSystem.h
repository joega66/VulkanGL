#pragma once
#include "System.h"

class EditorControllerSystem : public ISystem
{
	SYSTEM(EditorControllerSystem);
public:
	virtual void Update(class Scene& Scene) override;
};