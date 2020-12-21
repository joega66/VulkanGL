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
	EShadowType _ShadowType = EShadowType::None;
	glm::vec3 _Color = glm::vec3(1.0f);
	float _Intensity = 1.0f;
	float _DepthBiasConstantFactor = 1.0f;
	float _DepthBiasSlopeFactor = 1.0f;
};

struct DirectionalLight : public Light, public Component
{
};

struct PointLight : public Light, public Component
{
	float _Range = 10.0f;
};