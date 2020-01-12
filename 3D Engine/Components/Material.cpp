#include "Material.h"
#include <DRM.h>

drm::ImageRef Material::Red;
drm::ImageRef Material::Green;
drm::ImageRef Material::Blue;
drm::ImageRef Material::White;

static drm::ImageRef GetDummy()
{
	static drm::ImageRef Dummy = nullptr;
	if (Dummy == nullptr)
	{
		uint8 DummyColor[] = { 234, 115, 79, 0 };
		Dummy = drm::CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, DummyColor);
	}
	return Dummy;
}

Material::Material()
	: Diffuse(GetDummy())
	, Specular(GetDummy())
	, Opacity(GetDummy())
	, Bump(GetDummy())
{
}

bool Material::HasSpecularMap() const
{
	return Specular != GetDummy();
}

bool Material::IsMasked() const
{ 
	return Opacity != GetDummy(); 
}

bool Material::HasBumpMap() const
{
	return Bump != GetDummy();
}

drm::DescriptorSetRef Material::CreateDescriptorSet() const
{
	static const SamplerState LinearSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	static const SamplerState BumpSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };

	drm::DescriptorSetRef DescriptorSet = drm::CreateDescriptorSet();

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
