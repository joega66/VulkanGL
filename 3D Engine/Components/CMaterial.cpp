#include "CMaterial.h"
#include <DRM.h>

drm::ImageRef CMaterial::Red;
drm::ImageRef CMaterial::Green;
drm::ImageRef CMaterial::Blue;
drm::ImageRef CMaterial::White;

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

CMaterial::CMaterial()
	: Diffuse(GetDummy())
	, Specular(GetDummy())
	, Opacity(GetDummy())
	, Bump(GetDummy())
{
}

bool CMaterial::HasSpecularMap() const
{
	return Specular != GetDummy();
}

bool CMaterial::IsMasked() const
{ 
	return Opacity != GetDummy(); 
}

bool CMaterial::HasBumpMap() const
{
	return Bump != GetDummy();
}

drm::DescriptorSetRef CMaterial::CreateDescriptorSet() const
{
	static const SamplerState LinearSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	static const SamplerState BumpSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };

	drm::DescriptorSetRef DescriptorSet = drm::CreateDescriptorSet();

	DescriptorSet->Write(Diffuse, LinearSampler, ShaderBinding(1));
	DescriptorSet->Write(Specular, LinearSampler, ShaderBinding(2));
	DescriptorSet->Write(Opacity, LinearSampler, ShaderBinding(3));
	DescriptorSet->Write(Bump, BumpSampler, ShaderBinding(4));

	return DescriptorSet;
}

SpecializationInfo CMaterial::CreateSpecializationInfo() const
{
	SpecializationInfo SpecInfo;
	SpecInfo.Add(0, HasSpecularMap());
	SpecInfo.Add(1, IsMasked());
	return SpecInfo;
}
