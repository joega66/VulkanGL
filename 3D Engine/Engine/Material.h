#pragma once
#include <GPU/GPUDefinitions.h>
#include <GPU/GPUShader.h>

class gpu::Device;

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
		gpu::TextureID BaseColor;
		gpu::TextureID MetallicRoughness;
		gpu::TextureID Normal;
		gpu::TextureID Emissive;
		gpu::SamplerID Sampler;
		float Metallic;
		float Roughness;
		glm::vec3 EmissiveFactor;
	};

	Material() = default;

	Material(
		gpu::Device& Device,
		EMaterialMode MaterialMode,
		const gpu::Image* BaseColor,
		const gpu::Image* MetallicRoughness,
		const gpu::Image* Normal,
		const gpu::Image* Emissive,
		float Metallic,
		float Roughness,
		const glm::vec3& EmissiveFactor
	);

	inline bool IsMasked() const { return MaterialMode == EMaterialMode::Masked; };
	inline const PushConstants& GetPushConstants() const { return PushConstants; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return mSpecializationInfo; }

private:
	EMaterialMode MaterialMode;
	PushConstants PushConstants;
	SpecializationInfo mSpecializationInfo;
};