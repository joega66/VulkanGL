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
	static drm::ImageRef Black;
	/** Dummy texture for "empty" descriptors. */
	static drm::ImageRef Dummy;
	
	drm::DescriptorSetRef CreateDescriptorSet(class DRMDevice& Device) const;

	SpecializationInfo CreateSpecializationInfo() const;
};