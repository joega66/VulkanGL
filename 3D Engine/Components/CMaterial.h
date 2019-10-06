#pragma once
#include <ECS/Component.h>
#include <DRMResource.h>

struct CMaterial : Component<CMaterial>
{
	CMaterial();

	drm::ImageRef Diffuse;
	drm::ImageRef Specular;
	drm::ImageRef Opacity;

	bool IsMasked() const;

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
	static drm::ImageRef White;
};