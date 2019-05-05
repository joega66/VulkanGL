#pragma once
#include "Component.h"
#include "../DRMResource.h"

struct CMaterial : Component<CMaterial>
{
	std::variant<glm::vec4, drm::ImageRef> Diffuse = glm::vec4(1.0f);
	drm::ImageRef Normal;
};