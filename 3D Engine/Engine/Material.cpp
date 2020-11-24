#include "Material.h"
#include <Engine/AssetManager.h>
#include <GPU/GPU.h>

Material::Material(
	gpu::Device& Device,
	EMaterialMode MaterialMode,
	gpu::Image* BaseColor,
	gpu::Image* MetallicRoughness,
	gpu::Image* Normal,
	gpu::Image* Emissive,
	float Metallic,
	float Roughness,
	const glm::vec3& EmissiveFactor
) : MaterialMode(MaterialMode)
{
	const gpu::Sampler sampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear });

	PushConstants.BaseColor = BaseColor->GetTextureID(sampler);
	PushConstants.MetallicRoughness = MetallicRoughness ? MetallicRoughness->GetTextureID(sampler) : gpu::TextureID{};
	PushConstants.Normal = Normal ? Normal->GetTextureID(sampler) : gpu::TextureID{};
	PushConstants.Emissive = Emissive ? Emissive->GetTextureID(sampler) : gpu::TextureID{};

	PushConstants.Metallic = Metallic;
	PushConstants.Roughness = Roughness;
	PushConstants.EmissiveFactor = EmissiveFactor;

	mSpecializationInfo.Add(0, MetallicRoughness != nullptr);
	mSpecializationInfo.Add(1, MaterialMode == EMaterialMode::Masked);
	mSpecializationInfo.Add(2, Normal != nullptr);
	mSpecializationInfo.Add(3, Emissive != nullptr);
}