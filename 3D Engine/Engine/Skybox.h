#pragma once
#include <DRM.h>

class Skybox
{
public:
	Skybox(DRMDevice& Device, const std::array<const drm::Image*, 6>& Images, EFormat Format);
	
	inline const drm::Image& GetImage() const { return Image; }
	
private:
	/** Cubemap image. */
	drm::Image Image;
	/** Image faces. */
	const std::array<const drm::Image*, 6> Images;
};