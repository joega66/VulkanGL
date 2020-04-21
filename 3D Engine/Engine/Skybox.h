#pragma once
#include <DRM.h>

enum class CubemapFace
{
	Left = 0,	// +X
	Right,		// -X
	Up,			// +Y
	Down,		// -Y
	Front,		// +Z
	Back		// -Z
};

class Skybox
{
public:
	Skybox(DRMDevice& Device, const std::array<const drm::Image*, 6>& Images, EFormat Format);
	
	inline const drm::Image& GetImage() const { return Image; }
	inline const drm::Image* GetFace(CubemapFace Face) const { return Images[static_cast<uint64>(Face)]; }

private:
	/** Cubemap image. */
	drm::Image Image;
	/** Image faces. */
	std::array<const drm::Image*, 6> Images;
};