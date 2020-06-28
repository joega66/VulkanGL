#include "Material.h"
#include <Engine/AssetManager.h>
#include <GPU/GPU.h>

Material::Material(
	gpu::Device& Device,
	EMaterialMode MaterialMode,
	const gpu::Image* BaseColor,
	const gpu::Image* MetallicRoughness,
	const gpu::Image* Normal,
	const gpu::Image* Emissive,
	float Metallic,
	float Roughness,
	const glm::vec3& EmissiveFactor
) : MaterialMode(MaterialMode)
{
	const gpu::Sampler Sampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear });

	PushConstants.BaseColor = BaseColor->GetTextureID();
	PushConstants.MetallicRoughness = MetallicRoughness ? MetallicRoughness->GetTextureID().Get() : 0;
	PushConstants.Normal = Normal ? Normal->GetTextureID().Get() : 0;
	PushConstants.Emissive = Emissive ? Emissive->GetTextureID().Get() : 0;
	PushConstants.Sampler = Sampler.GetSamplerID();

	PushConstants.Metallic = Metallic;
	PushConstants.Roughness = Roughness;
	PushConstants.EmissiveFactor = EmissiveFactor;

	mSpecializationInfo.Add(0, MetallicRoughness != nullptr);
	mSpecializationInfo.Add(1, MaterialMode == EMaterialMode::Masked);
	mSpecializationInfo.Add(2, Normal != nullptr);
	mSpecializationInfo.Add(3, Emissive != nullptr);
}