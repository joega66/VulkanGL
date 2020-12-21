#pragma once
#include <GPU/GPUDefinitions.h>
#include <GPU/GPUShader.h>

namespace gpu { class Device; }

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
		gpu::TextureID _BaseColor;
		gpu::TextureID _MetallicRoughness;
		gpu::TextureID _Normal;
		gpu::TextureID _Emissive;
		float _Metallic;
		float _Roughness;
		glm::vec3 _EmissiveFactor;
	};

	Material() = default;

	Material(
		gpu::Device& device,
		EMaterialMode materialMode,
		gpu::Image* baseColor,
		gpu::Image* metallicRoughness,
		gpu::Image* normal,
		gpu::Image* emissive,
		float metallic,
		float roughness,
		const glm::vec3& emissiveFactor
	);

	inline bool IsMasked() const { return _MaterialMode == EMaterialMode::Masked; };
	inline const PushConstants& GetPushConstants() const { return _PushConstants; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return _SpecializationInfo; }

private:
	EMaterialMode _MaterialMode;
	PushConstants _PushConstants;
	SpecializationInfo _SpecializationInfo;
};