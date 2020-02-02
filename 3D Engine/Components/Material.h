#pragma once
#include <DRMResource.h>
#include <DRMShader.h>

struct MaterialSet
{
	drm::ImageRef Diffuse;
	SamplerState DiffuseSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::ImageRef Specular;
	SamplerState SpecularSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::ImageRef Opacity;
	SamplerState OpacitySampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::ImageRef Bump;
	SamplerState BumpSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };

	MaterialSet();

	static const std::vector<DescriptorTemplateEntry>& GetEntries()
	{
		static const std::vector<DescriptorTemplateEntry> Entries =
		{
			{ 0, 1, SampledImage },
			{ 1, 1, SampledImage },
			{ 2, 1, SampledImage },
			{ 3, 1, SampledImage }
		};
		return Entries;
	}
};

class Material
{
public:
	Material();

	MaterialSet MaterialSet;

	bool HasSpecularMap() const;

	bool IsMasked() const;

	bool HasBumpMap() const;

	SpecializationInfo CreateSpecializationInfo() const;

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
	static drm::ImageRef White;
	static drm::ImageRef Black;
	static drm::ImageRef Dummy;
};