#include "Material.h"
#include <Engine/AssetManager.h>
#include <GPU/GPU.h>

Material::Material(
	gpu::Device& device,
	EMaterialMode materialMode,
	gpu::Image* baseColor,
	gpu::Image* metallicRoughness,
	gpu::Image* normal,
	gpu::Image* emissive,
	float metallic,
	float roughness,
	const glm::vec3& emissiveFactor
) : _MaterialMode(materialMode)
{
	const gpu::Sampler sampler = device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear });

	_PushConstants._BaseColor = baseColor->GetTextureID(sampler);
	_PushConstants._MetallicRoughness = metallicRoughness ? metallicRoughness->GetTextureID(sampler) : gpu::TextureID{};
	_PushConstants._Normal = normal ? normal->GetTextureID(sampler) : gpu::TextureID{};
	_PushConstants._Emissive = emissive ? emissive->GetTextureID(sampler) : gpu::TextureID{};

	_PushConstants._Metallic = metallic;
	_PushConstants._Roughness = roughness;
	_PushConstants._EmissiveFactor = emissiveFactor;

	_SpecializationInfo.Add(0, metallicRoughness != nullptr);
	_SpecializationInfo.Add(1, materialMode == EMaterialMode::Masked);
	_SpecializationInfo.Add(2, normal != nullptr);
	_SpecializationInfo.Add(3, emissive != nullptr);
}