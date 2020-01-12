#include "Material.h"
#include <DRM.h>

drm::ImageRef Material::Red;
drm::ImageRef Material::Green;
drm::ImageRef Material::Blue;
drm::ImageRef Material::White;
drm::ImageRef Material::Dummy;

Material::Material()
	: Diffuse(Dummy)
	, Specular(Dummy)
	, Opacity(Dummy)
	, Bump(Dummy)
{
}

bool Material::HasSpecularMap() const
{
	return Specular != Dummy;
}

bool Material::IsMasked() const
{ 
	return Opacity != Dummy; 
}

bool Material::HasBumpMap() const
{
	return Bump != Dummy;
}

drm::DescriptorSetRef Material::CreateDescriptorSet(DRM& Device) const
{
	static const SamplerState LinearSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	static const SamplerState BumpSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };

	drm::DescriptorSetRef DescriptorSet = Device.CreateDescriptorSet();

	DescriptorSet->Write(Diffuse, LinearSampler, 0);
	DescriptorSet->Write(Specular, LinearSampler, 1);
	DescriptorSet->Write(Opacity, LinearSampler, 2);
	DescriptorSet->Write(Bump, BumpSampler, 3);
	DescriptorSet->Update();

	return DescriptorSet;
}

SpecializationInfo Material::CreateSpecializationInfo() const
{
	SpecializationInfo SpecInfo;
	SpecInfo.Add(0, HasSpecularMap());
	SpecInfo.Add(1, IsMasked());
	return SpecInfo;
}
