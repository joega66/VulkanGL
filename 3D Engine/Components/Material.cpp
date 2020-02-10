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
	float Roughness,
	float Metallicity)
	: Roughness(Roughness)
	, Metallicity(Metallicity)
{
	struct PBRUniformData
	{
		float Roughness;
		float Metallicity;
	} PBRUniformData = { Roughness, Metallicity };

	Descriptors.PBRUniform = Device.CreateBuffer(EBufferUsage::Uniform, sizeof(PBRUniformData), &PBRUniformData);
}

bool Material::HasSpecularMap() const
{
	return Descriptors.Specular != Dummy;
}

bool Material::IsMasked() const
{ 
	return Descriptors.Opacity != Dummy;
}

bool Material::HasBumpMap() const
{
	return Descriptors.Bump != Dummy;
}

SpecializationInfo Material::CreateSpecializationInfo() const
{
	SpecializationInfo SpecInfo;
	SpecInfo.Add(0, HasSpecularMap());
	SpecInfo.Add(1, IsMasked());
	return SpecInfo;
}

MaterialDescriptors::MaterialDescriptors()
	: Diffuse(Material::Dummy)
	, Specular(Material::Dummy)
	, Opacity(Material::Dummy)
	, Bump(Material::Dummy)
{
}