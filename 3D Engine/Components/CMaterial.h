#pragma once
#include <ECS/Component.h>
#include <DRMResource.h>

// @todo Make these pipeline constants set by the material.

struct CMaterial : Component<CMaterial>
{
	drm::ImageRef Diffuse;
	drm::ImageRef Specular;
	drm::ImageRef Opacity;

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
	static drm::ImageRef White;
};