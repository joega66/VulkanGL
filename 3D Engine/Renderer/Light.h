#pragma once
#include <Platform/Platform.h>

struct PointLightProxy
{
	glm::vec3 Position;
	float Intensity;
	glm::vec3 Color;
	float Range;
};

struct DirectionalLightProxy
{
	glm::vec3 Color;
	float Intensity;
	glm::vec3 Direction;
	int _Pad1;
};