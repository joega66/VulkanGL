#include "Material.h"
#include <DRM.h>

drm::ImageRef Material::Red;
drm::ImageRef Material::Green;
drm::ImageRef Material::Blue;
drm::ImageRef Material::White;
drm::ImageRef Material::Black;
drm::ImageRef Material::Dummy;

Material::Material(
	DRMDevice& Device,
	const MaterialDescriptors& InDescriptors,
	EMaterialMode MaterialMode,
	float Roughness,
	float Metallicity)
	: Descriptors(InDescriptors)
	, MaterialMode(MaterialMode)
	, Roughness(Roughness)
	, Metallicity(Metallicity)
{
	struct PBRUniformData
	{
		float Roughness;
		float Metallicity;
	} PBRUniformData = { Roughness, Metallicity };

	Descriptors.PBRUniform = Device.CreateBuffer(EBufferUsage::Uniform, sizeof(PBRUniformData), &PBRUniformData);

	SpecializationInfo.Add(0, Descriptors.MetallicRoughness != Material::Dummy);
	SpecializationInfo.Add(1, MaterialMode == EMaterialMode::Masked);
}

MaterialDescriptors::MaterialDescriptors()
	: BaseColor(Material::Dummy)
	, MetallicRoughness(Material::Dummy)
{
}