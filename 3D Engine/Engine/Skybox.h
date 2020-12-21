#pragma once
#include <GPU/GPU.h>

enum CubemapFace
{
	CubemapFace_Left,		// +X
	CubemapFace_Right,		// -X
	CubemapFace_Up,			// +Y
	CubemapFace_Down,		// -Y
	CubemapFace_Front,		// +Z
	CubemapFace_Back,		// -Z
	CubemapFace_Begin = CubemapFace_Left,
	CubemapFace_End = CubemapFace_Back + 1
};

class Skybox
{
public:
	Skybox(gpu::Device& device, const std::array<gpu::Image*, 6>& images, EFormat format);
	
	inline gpu::Image& GetImage() { return _Image; }
	inline std::array<gpu::Image*, 6>& GetFaces() { return _Images; }

	static const std::string _CubemapFaces[6];
	static const std::string _CubemapStems[6];

private:
	/** Cubemap image. */
	gpu::Image _Image;
	/** Image faces. */
	std::array<gpu::Image*, 6> _Images;
};