#pragma once
#include <ECS/Component.h>

enum class EShadowType
{
	None,
	Hard,
	Soft,
};

struct Light
{
	EShadowType ShadowType = EShadowType::None;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
	float DepthBiasConstantFactor = 1.0f;
	float DepthBiasSlopeFactor = 1.0f;
};

struct DirectionalLight : public Light, public Component
{
	glm::vec3 Direction;
};

struct PointLight : public Light, public Component
{
	float Range = 10.0f;
};