#include "Material.h"
#include <Engine/AssetManager.h>
#include <DRM.h>

Material::Material(
	DRMDevice& Device,
	EMaterialMode MaterialMode,
	const drm::Image* BaseColor,
	const drm::Image* MetallicRoughness,
	const drm::Image* Normal,
	const drm::Image* Emissive,
	float Metallic,
	float Roughness,
	const glm::vec3& EmissiveFactor
) : MaterialMode(MaterialMode)
{
	const drm::Sampler Sampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear });

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