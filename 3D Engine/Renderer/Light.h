#pragma once
#include <Platform/Platform.h>

struct PointLightProxy
{
	glm::vec3 Position;
	float Intensity;
	glm::vec3 Color;
	float Range;
};