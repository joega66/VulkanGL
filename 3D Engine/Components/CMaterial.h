#pragma once
#include "Component.h"
#include "../DRMResource.h"

struct CMaterial : Component<CMaterial>
{
	drm::ImageRef Diffuse;
	drm::ImageRef Normal;

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
};