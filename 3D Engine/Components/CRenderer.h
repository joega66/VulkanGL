#pragma once
#include <ECS/Component.h>

struct CRenderer : Component<CRenderer>
{
	// Hide/Show the 3D object
	bool bVisible = true;
};