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
#include "GlobalRenderData.h"

SceneRenderer::SceneRenderer(Engine& Engine)
	: Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
	, ECS(Engine.ECS)
	, Assets(Engine.Assets)
{
}

void SceneRenderer::Render(SceneProxy& Scene)
{
	GlobalRenderData& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Graphics);

	RenderDepthPrepass(Scene, CmdList);

	RenderShadowDepths(Scene, CmdList);

	VCTLightingCache& VCTLightingCache = GlobalData.VCTLightingCache;
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

	ECS.GetSingletonComponent<ImGuiRenderData>().Render(CmdList);

	CmdList.EndRenderPass();

	Present(CmdList);

	Device.EndFrame();
}

void SceneRenderer::Present(drm::CommandList& CmdList)
{
	auto& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();

	const uint32 ImageIndex = ECS.GetSingletonComponent<GlobalRenderData>().Surface.AcquireNextImage(Device);
	const drm::Image& PresentImage = GlobalData.Surface.GetImage(ImageIndex);

	ImageMemoryBarrier Barrier(PresentImage, EAccess::MemoryRead, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList.BlitImage(GlobalData.SceneColor, EImageLayout::TransferSrcOptimal, PresentImage, EImageLayout::TransferDstOptimal, EFilter::Nearest);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::MemoryRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::Present;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &Barrier);

	GlobalData.Surface.Present(Device, ImageIndex, CmdList);
}