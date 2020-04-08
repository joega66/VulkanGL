#include "Material.h"
#include <Engine/AssetManager.h>
#include <DRM.h>

Material::Material(
	DRMDevice& Device,
	EMaterialMode MaterialMode,
	const drm::Image* BaseColor,
	const drm::Image* MetallicRoughness,
	const drm::Image* Normal,
	float Metallic,
	float Roughness)
	: MaterialMode(MaterialMode)
{
	const drm::Sampler Sampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear });

	PushConstants.BaseColorIndex = Device.GetSampledImages().Add(BaseColor->GetImageView(), Sampler);
	PushConstants.MetallicRoughnessIndex = MetallicRoughness ? Device.GetSampledImages().Add(MetallicRoughness->GetImageView(), Sampler) : 0;
	PushConstants.NormalIndex = Normal ? Device.GetSampledImages().Add(Normal->GetImageView(), Sampler) : 0;

	PushConstants.Metallic = Metallic;
	PushConstants.Roughness = Roughness;
	
	mSpecializationInfo.Add(0, MetallicRoughness != nullptr);
	mSpecializationInfo.Add(1, MaterialMode == EMaterialMode::Masked);
	mSpecializationInfo.Add(2, Normal != nullptr);
}