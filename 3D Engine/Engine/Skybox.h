#pragma once
#include <DRM.h>

class Skybox
{
public:
	Skybox(DRMDevice& Device, const std::array<std::string, 6>& Files, EFormat Format);
	
	inline const drm::Image& GetImage() const { return Image; }
	
private:
	drm::Image Image;
};