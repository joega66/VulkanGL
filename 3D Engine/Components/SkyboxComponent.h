#pragma once
#include <Engine/Skybox.h>
#include <ECS/Component.h>

class SkyboxComponent : public Component
{
public:
	SkyboxComponent(Skybox* skybox)
		: _Skybox(skybox)
	{
	}

	Skybox* _Skybox = nullptr;
};