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
		float Metallic;
		float Roughness;
	};

	Material() = default;

	Material(
		DRMDevice& Device,
		EMaterialMode MaterialMode,
		const drm::Image* BaseColor,
		const drm::Image* MetallicRoughness,
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

	static drm::Image Red;
	static drm::Image Green;
	static drm::Image Blue;
	static drm::Image White;
	static drm::Image Black;

	static void CreateDebugMaterials(DRMDevice& Device);

private:
	EMaterialMode MaterialMode;
	PushConstants PushConstants;
	SpecializationInfo mSpecializationInfo;
};