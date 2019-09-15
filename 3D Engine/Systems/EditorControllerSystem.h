#pragma once
#include <ECS/System.h>

class EditorControllerSystem : public ISystem
{
	SYSTEM(EditorControllerSystem);
public:
	virtual void Start(class Scene& Scene) override;
	virtual void Update(class Scene& Scene) override;
};