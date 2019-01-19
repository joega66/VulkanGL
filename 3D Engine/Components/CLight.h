#pragma once
#include "Component.h"

enum class ELightType
{
	Point,
	Directional,
};

enum class EShadowType
{
	None,
	Hard,
	Soft,
};

struct CLight : public Component<CLight>
{
	ELightType LightType = ELightType::Point;
	EShadowType ShadowType = EShadowType::None;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
	float Range = 10.0f;
};