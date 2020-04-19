#pragma once
#include "Voxels.h"
#include <ECS/Component.h>

/** Global render data, typically bound to set #0, visible to all passes. */
class GlobalRenderData : public Component
{
public:
	GlobalRenderData(class Engine& Engine);

	VCTLightingCache VCTLightingCache;
};