#pragma once
#include "Component.h"

struct CRenderer : Component<CRenderer>
{
	// Hide/Show the 3D object
	bool bVisible = true;
};