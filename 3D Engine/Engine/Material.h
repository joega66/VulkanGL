#pragma once
#include <DRMResource.h>
#include <DRMShader.h>

struct MaterialDescriptors
{
	drm::ImageRef BaseColor;
	SamplerState BaseColorSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::ImageRef MetallicRoughness;
	SamplerState MetallicRoughnessSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	drm::BufferRef PBRUniform;

	MaterialDescriptors();

	static const std::vector<DescriptorTemplateEntry>& GetEntries()
	{
		static const std::vector<DescriptorTemplateEntry> Entries =
		{
			{ 0, 1, SampledImage },
			{ 1, 1, SampledImage },
			{ 2, 1, UniformBuffer },
		};
		return Entries;
	}
};

enum class EMaterialMode
{
	Opaque,
	Masked,
};

class Material
{
public:
	Material() = default;

	Material(
		class DRMDevice& Device,
		const MaterialDescriptors& Descriptors,
		EMaterialMode MaterialMode,
		float Roughness,
		float Metallicity
	);

	MaterialDescriptors Descriptors;

	drm::DescriptorSetRef DescriptorSet;

	inline bool IsMasked() const { return MaterialMode == EMaterialMode::Masked; };
	inline float GetRoughness() const { return Roughness; }
	inline float GetMetallicity() const { return Metallicity; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return SpecializationInfo; }

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
	static drm::ImageRef White;
	static drm::ImageRef Black;
	static drm::ImageRef Dummy;

	static void CreateDebugMaterials(DRMDevice& Device);

private:
	EMaterialMode MaterialMode;
	float Roughness = 0.0f;
	float Metallicity = 0.0f;
	SpecializationInfo SpecializationInfo;
};