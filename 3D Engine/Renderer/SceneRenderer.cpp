#include "SceneRenderer.h"
#include <DRMShader.h>
#include <Engine/Engine.h>
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
	, Surface(Engine.Surface)
	, ECS(Engine.ECS)
	, Assets(Engine.Assets)
{
}

void SceneRenderer::Render(CameraProxy& Camera)
{
	RenderSettings& Settings = ECS.GetSingletonComponent<RenderSettings>();
	GlobalRenderData& GlobalData = ECS.GetSingletonComponent<GlobalRenderData>();
	VCTLightingCache& VCTLightingCache = GlobalData.VCTLightingCache;

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Graphics);

	ImageMemoryBarrier ImageBarrier
	{
		Camera.SceneColor,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	};

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &ImageBarrier);

	CmdList.ClearColorImage(Camera.SceneColor, EImageLayout::TransferDstOptimal, {});

	ImageBarrier.SrcAccessMask = EAccess::TransferWrite;
	ImageBarrier.DstAccessMask = EAccess::ShaderWrite;
	ImageBarrier.OldLayout = EImageLayout::TransferDstOptimal;
	ImageBarrier.NewLayout = EImageLayout::General;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::ComputeShader, 0, nullptr, 1, &ImageBarrier);

	RenderGBufferPass(Camera, CmdList);

	RenderShadowDepths(Camera, CmdList);

	if (Settings.bVoxelize)
	{
		VCTLightingCache.Render(ECS, Camera, CmdList);
		
		Settings.bVoxelize = false;
	}
	
	if (Settings.bDrawVoxels && VCTLightingCache.IsDebuggingEnabled())
	{
		VCTLightingCache.RenderVisualization(Camera, CmdList);
	}
	else
	{
		VCTLightingCache.PreLightingPass(CmdList);

		ComputeLightingPass(Camera, CmdList);
	}

	CmdList.BeginRenderPass(Camera.SceneRP);

	RenderSkybox(Camera, CmdList);

	CmdList.EndRenderPass();

	const uint32 ImageIndex = Surface.AcquireNextImage(Device);
	const drm::Image& DisplayImage = Surface.GetImage(ImageIndex);
	ImageMemoryBarrier Barrier{ DisplayImage, EAccess::MemoryRead, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General };

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &Barrier);

	ComputePostProcessing(DisplayImage, Camera, CmdList);

	Barrier.SrcAccessMask = EAccess::ShaderWrite;
	Barrier.DstAccessMask = EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite;
	Barrier.OldLayout = EImageLayout::General;
	Barrier.NewLayout = EImageLayout::ColorAttachmentOptimal;

	CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ColorAttachmentOutput, 0, nullptr, 1, &Barrier);

	ECS.GetSingletonComponent<ImGuiRenderData>().Render(Device, CmdList, Camera.UserInterfaceRP[ImageIndex]);

	Surface.Present(Device, ImageIndex, CmdList);

	Device.EndFrame();
}