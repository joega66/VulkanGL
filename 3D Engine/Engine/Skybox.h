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
	Skybox(gpu::Device& Device, const std::array<gpu::Image*, 6>& Images, EFormat Format);
	
	inline gpu::Image& GetImage() { return Image; }
	inline std::array<gpu::Image*, 6>& GetFaces() { return Images; }

	static const std::string CubemapFaces[6];
	static const std::string CubemapStems[6];

private:
	/** Cubemap image. */
	gpu::Image Image;
	/** Image faces. */
	std::array<gpu::Image*, 6> Images;
};