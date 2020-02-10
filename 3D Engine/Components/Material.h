#pragma once
#include <DRMResource.h>
#include <DRMShader.h>

struct MaterialDescriptors
{
	drm::ImageRef Diffuse;
	SamplerState DiffuseSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::ImageRef Specular;
	SamplerState SpecularSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::ImageRef Opacity;
	SamplerState OpacitySampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::ImageRef Bump;
	SamplerState BumpSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::BufferRef PBRUniform;

	MaterialDescriptors();

	static const std::vector<DescriptorTemplateEntry>& GetEntries()
	{
		static const std::vector<DescriptorTemplateEntry> Entries =
		{
			{ 0, 1, SampledImage },
			{ 1, 1, SampledImage },
			{ 2, 1, SampledImage },
			{ 3, 1, SampledImage },
			{ 4, 1, UniformBuffer },
		};
		return Entries;
	}
};

class Material
{
public:
	Material() = default;

	Material(
		class DRMDevice& Device,
		float Roughness,
		float Metallicity
	);

	MaterialDescriptors Descriptors;

	bool HasSpecularMap() const;

	bool IsMasked() const;

	bool HasBumpMap() const;

	SpecializationInfo CreateSpecializationInfo() const;

	inline float GetRoughness() const { return Roughness; }
	inline float GetMetallicity() const { return Metallicity; }

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
	static drm::ImageRef White;
	static drm::ImageRef Black;
	static drm::ImageRef Dummy;

private:
	float Roughness = 0.0f;
	float Metallicity = 0.0f;
};