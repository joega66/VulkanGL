#include "GlobalRenderData.h"
#include <Engine/Engine.h>
#include <Engine/Screen.h>

GlobalRenderData::GlobalRenderData(Engine& Engine)
	: SkyboxDescriptorSet(Engine.Device)
	, VCTLightingCache(Engine)
{
}