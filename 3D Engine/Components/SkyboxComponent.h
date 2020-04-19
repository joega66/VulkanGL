#pragma once
#include <Engine/Skybox.h>

class SkyboxComponent
{
public:
	SkyboxComponent(const Skybox* Skybox)
		: Skybox(Skybox)
	{
	}

	const Skybox* Skybox = nullptr;
};