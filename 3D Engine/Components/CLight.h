#pragma once
#include <ECS/Component.h>

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
	EShadowType ShadowType = EShadowType::None;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
};

struct CDirectionalLight : public CLight
{
	glm::vec3 Direction;
};

struct CPointLight : public CLight
{
	float Range = 10.0f;
};