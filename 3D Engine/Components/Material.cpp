#include "Material.h"
#include <DRM.h>

drm::ImageRef Material::Red;
drm::ImageRef Material::Green;
drm::ImageRef Material::Blue;
drm::ImageRef Material::White;
drm::ImageRef Material::Black;
drm::ImageRef Material::Dummy;

Material::Material()
{
}

bool Material::HasSpecularMap() const
{
	return MaterialSet.Specular != Dummy;
}

bool Material::IsMasked() const
{ 
	return MaterialSet.Opacity != Dummy;
}

bool Material::HasBumpMap() const
{
	return MaterialSet.Bump != Dummy;
}

SpecializationInfo Material::CreateSpecializationInfo() const
{
	SpecializationInfo SpecInfo;
	SpecInfo.Add(0, HasSpecularMap());
	SpecInfo.Add(1, IsMasked());
	return SpecInfo;
}

MaterialSet::MaterialSet()
	: Diffuse(Material::Dummy)
	, Specular(Material::Dummy)
	, Opacity(Material::Dummy)
	, Bump(Material::Dummy)
{
}