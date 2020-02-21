#pragma once
#include <DRMDefinitions.h>
#include <DRMShader.h>

struct MaterialDescriptors
{
	const drm::Image* BaseColor;
	const drm::Sampler* BaseColorSampler;
	const drm::Image* MetallicRoughness;
	const drm::Sampler* MetallicRoughnessSampler;
	const drm::Buffer* PBRUniform;

	MaterialDescriptors();

	static const std::vector<DescriptorBinding>& GetBindings()
	{
		static const std::vector<DescriptorBinding> Bindings =
		{
			{ 0, 1, SampledImage },
			{ 1, 1, SampledImage },
			{ 2, 1, UniformBuffer },
		};
		return Bindings;
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

	drm::DescriptorSet DescriptorSet;

	inline bool IsMasked() const { return MaterialMode == EMaterialMode::Masked; };
	inline float GetRoughness() const { return Roughness; }
	inline float GetMetallicity() const { return Metallicity; }
	inline const SpecializationInfo& GetSpecializationInfo() const { return SpecializationInfo; }

	static drm::Image Red;
	static drm::Image Green;
	static drm::Image Blue;
	static drm::Image White;
	static drm::Image Black;
	static drm::Image Dummy;

	static void CreateDebugMaterials(DRMDevice& Device);

private:
	drm::Buffer PBRUniform;
	EMaterialMode MaterialMode;
	float Roughness = 0.0f;
	float Metallicity = 0.0f;
	SpecializationInfo SpecializationInfo;
};