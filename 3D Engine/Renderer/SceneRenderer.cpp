#include "SceneRenderer.h"
#include <DRMShader.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Components/RenderSettings.h>
#include <Systems/UserInterface.h>
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include "Voxels.h"

SceneRenderer::SceneRenderer(Engine& Engine)
	: Device(Engine.Device)
	, ShaderLibrary(Engine.ShaderLibrary)
	, Surface(Engine.Surface)
	, ECS(Engine.ECS)
	, Assets(Engine.Assets)
	, _Camera(Engine.Camera)
{
}

void SceneRenderer::Render(CameraProxy& Camera)
{
	RenderSettings& Settings = ECS.GetSingletonComponent<RenderSettings>();
	auto& VCTLighting = ECS.GetSingletonComponent<VCTLightingCache>();

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Graphics);

	if (Settings.bRayTracing)
	{
		ComputeRayTracing(Camera, CmdList);
	}
	else
	{
		ClearSceneColor(Camera, CmdList);

		RenderGBufferPass(Camera, CmdList);

		RenderShadowDepths(Camera, CmdList);

		if (Settings.bVoxelize)
		{
			VCTLighting.Render(ECS, Camera, CmdList);

			Settings.bVoxelize = false;
		}

		if (Settings.VoxelDebugMode != EVoxelDebugMode::None && VCTLighting.IsDebuggingEnabled())
		{
			VCTLighting.RenderVisualization(Camera, CmdList, Settings.VoxelDebugMode);
		}
		else
		{
			ComputeLightingPass(Camera, CmdList);

			ComputeIndirectLightingPass(Camera, CmdList);
		}

		CmdList.BeginRenderPass(Camera.SceneRP);

		RenderSkybox(Camera, CmdList);

		CmdList.EndRenderPass();
	}
	
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

void SceneRenderer::ClearSceneColor(CameraProxy& camera, drm::CommandList& cmdList)
{
	ImageMemoryBarrier imageBarrier
	{
		camera.SceneColor,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	};

	cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &imageBarrier);

	cmdList.ClearColorImage(camera.SceneColor, EImageLayout::TransferDstOptimal, {});

	imageBarrier.SrcAccessMask = EAccess::TransferWrite;
	imageBarrier.DstAccessMask = EAccess::ShaderWrite;
	imageBarrier.OldLayout = EImageLayout::TransferDstOptimal;
	imageBarrier.NewLayout = EImageLayout::General;

	cmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
}