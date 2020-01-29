#pragma once

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

struct CLight
{
	EShadowType ShadowType = EShadowType::None;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
	float DepthBiasConstantFactor = 1.0f;
	float DepthBiasSlopeFactor = 1.0f;
};

struct DirectionalLight : public CLight
{
	glm::vec3 Direction;
};

struct CPointLight : public CLight
{
	float Range = 10.0f;
};