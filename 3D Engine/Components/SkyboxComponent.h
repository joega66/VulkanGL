#pragma once
#include <Engine/Skybox.h>
#include <ECS/Component.h>

class SkyboxComponent : public Component
{
public:
	SkyboxComponent(const Skybox* Skybox)
		: Skybox(Skybox)
	{
	}

	const Skybox* Skybox = nullptr;
};