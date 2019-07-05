#pragma once
#include "Component.h"
#include "../DRMResource.h"

struct CMaterial : Component<CMaterial>
{
	drm::ImageRef Diffuse;
	drm::ImageRef Normal;
};