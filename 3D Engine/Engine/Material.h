#pragma once
#include <DRMDefinitions.h>
#include <DRMShader.h>

class DRMDevice;

enum class EMaterialMode
{
	Opaque,
	Masked,
};

class Material
{
public:
	struct PushConstants
	{
		drm::TextureID BaseColor;
		drm::TextureID MetallicRoughness;
		drm::TextureID Normal;
		drm::TextureID Emissive;
		drm::SamplerID Sampler;
		float Metallic;
		float Roughness;
		glm::vec3 EmissiveFactor;
	};

	Material() = default;

	Material(
		DRMDevice& Device,
		EMaterialMode MaterialMode,
		const drm::Image* BaseColor,
		const drm::Image* MetallicRoughness,
		const drm::Image* Normal,
		const drm::Image* Emissive,
		float Metallic,
		float Roughness,
		const glm::vec3& EmissiveFactor
	);

	inline bool IsMasked() const { return MaterialMode == EMaterialMode::Masked; };
	inline const PushConstants& GetPushConstants() const { return PushConstants; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return mSpecializationInfo; }
	inline const PushConstantRange& GetPushConstantRange() const 
	{
		static PushConstantRange PushConstantRange = { EShaderStage::Fragment, sizeof(PushConstants) };
		return PushConstantRange;
	}

private:
	EMaterialMode MaterialMode;
	PushConstants PushConstants;
	SpecializationInfo mSpecializationInfo;
};