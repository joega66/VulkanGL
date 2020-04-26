#pragma once
#include <Engine/Skybox.h>
#include <ECS/Component.h>

class SkyboxComponent : public Component
{
public:
	SkyboxComponent(Skybox* Skybox)
		: Skybox(Skybox)
	{
	}

	Skybox* Skybox = nullptr;
};