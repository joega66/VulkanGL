#pragma once
#include <ECS/Component.h>
#include <DRMResource.h>

struct CMaterial : Component<CMaterial>
{
	CMaterial();

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
};