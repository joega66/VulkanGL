#pragma once
#include <ECS/System.h>

class ShadowSystem : public IRenderSystem
{
public:
	void Start(Engine& engine) override;
	void Update(Engine& engine) override;
};