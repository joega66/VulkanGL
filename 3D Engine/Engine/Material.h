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
		uint32 BaseColor;
		uint32 MetallicRoughness;
		uint32 Normal;
		uint32 Emissive;
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