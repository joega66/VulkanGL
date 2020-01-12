#pragma once
#include <DRMResource.h>
#include <DRMShader.h>

class Material
{
public:
	Material();

	drm::ImageRef Diffuse;
	drm::ImageRef Specular;
	drm::ImageRef Opacity;
	drm::ImageRef Bump;

	bool HasSpecularMap() const;

	bool IsMasked() const;

	bool HasBumpMap() const;

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
	static drm::ImageRef White;

	drm::DescriptorSetRef CreateDescriptorSet() const;

	SpecializationInfo CreateSpecializationInfo() const;
};