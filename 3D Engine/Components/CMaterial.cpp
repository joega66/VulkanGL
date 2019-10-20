#include "CMaterial.h"
#include <DRM.h>

drm::ImageRef CMaterial::Red;
drm::ImageRef CMaterial::Green;
drm::ImageRef CMaterial::Blue;
drm::ImageRef CMaterial::White;

static drm::ImageRef GetDummy()
{
	static drm::ImageRef Dummy = nullptr;
	if (Dummy == nullptr)
	{
		uint8 DummyColor[] = { 234, 115, 79, 0 };
		Dummy = drm::CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled, DummyColor);
	}
	return Dummy;
}

CMaterial::CMaterial()
	: Diffuse(GetDummy())
	, Specular(GetDummy())
	, Opacity(GetDummy())
	, Bump(GetDummy())
{
}

bool CMaterial::HasSpecularMap() const
{
	return Specular != GetDummy();
}

bool CMaterial::IsMasked() const
{ 
	return Opacity != GetDummy(); 
}

bool CMaterial::HasBumpMap() const
{
	return Bump != GetDummy();
}
