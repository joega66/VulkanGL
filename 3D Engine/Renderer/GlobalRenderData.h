#pragma once
#include "Voxels.h"

/** Global render data, typically bound to set #0, visible to all passes. */
class GlobalRenderData
{
public:
	GlobalRenderData(class Engine& Engine);

	VCTLightingCache VCTLightingCache;
};