#include "SceneRenderer.h"
#include <DRMShader.h>
#include <Engine/Engine.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Components/RenderSettings.h>
#include <Systems/UserInterface.h>
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include "GlobalRenderResources.h"

SceneRenderer::SceneRenderer(Engine& Engine)
	: Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
	, ECS(Engine.ECS)
	, Assets(Engine.Assets)
{
}

void SceneRenderer::Render(UserInterface& UserInterface, SceneProxy& Scene)
{
	GlobalRenderResources& GlobalResources = ECS.GetSingletonComponent<GlobalRenderResources>();

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Graphics);

	RenderDepthPrepass(Scene, CmdList);

	RenderShadowDepths(Scene, CmdList);

	VCTLightingCache& VCTLightingCache = GlobalResources.VCTLightingCache;
	VCTLightingCache.Render(Scene, CmdList);

	if (ECS.GetSingletonComponent<RenderSettings>().DrawVoxels && VCTLightingCache.IsDebuggingEnabled())
	{
		VCTLightingCache.RenderVisualization(Scene, CmdList);
	}
	else
	{
		RenderLightingPass(Scene, CmdList);
	}
	
	RenderSkybox(Scene, CmdList);

	UserInterface.Render(Device, GlobalResources.LightingRP, CmdList);

	CmdList.EndRenderPass();

	Present(CmdList);

	Device.EndFrame();
}

void SceneRenderer::Present(drm::CommandList& CmdList)
{
	auto& GlobalResources = ECS.GetSingletonComponent<GlobalRenderResources>();

	const uint32 ImageIndex = ECS.GetSingletonComponent<GlobalRenderResources>().Surface.AcquireNextImage(Device);
	const drm::Image& PresentImage = GlobalResources.Surface.GetImage(ImageIndex);

	ImageMemoryBarrier Barrier(PresentImage, EAccess::MemoryRead, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList.BlitImage(GlobalResources.SceneColor, EImageLayout::TransferSrcOptimal, PresentImage, EImageLayout::TransferDstOptimal, EFilter::Nearest);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::MemoryRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::Present;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &Barrier);

	GlobalResources.Surface.Present(Device, ImageIndex, CmdList);
}