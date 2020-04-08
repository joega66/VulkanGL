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
		uint32 BaseColorIndex;
		uint32 MetallicRoughnessIndex;
		uint32 NormalIndex;
		float Metallic;
		float Roughness;
	};

	Material() = default;

	Material(
		DRMDevice& Device,
		EMaterialMode MaterialMode,
		const drm::Image* BaseColor,
		const drm::Image* MetallicRoughness,
		const drm::Image* Normal,
		float Metallic,
		float Roughness
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