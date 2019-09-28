#pragma once
#include <ECS/Component.h>
#include "DRMResource.h"

struct CMaterial : Component<CMaterial>
{
	drm::ImageRef Diffuse;
	drm::ImageRef Normal;

	static drm::ImageRef Red;
	static drm::ImageRef Green;
	static drm::ImageRef Blue;
	static drm::ImageRef White;
};