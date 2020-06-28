#include "SceneRenderer.h"
#include <GPU/GPUShader.h>
#include <Engine/Engine.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Components/RenderSettings.h>
#include <Systems/UserInterface.h>
#include "FullscreenQuad.h"
#include "ShadowProxy.h"

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
	const RenderSettings& Settings = ECS.GetSingletonComponent<RenderSettings>();

	gpu::CommandList CmdList = Device.CreateCommandList(EQueue::Graphics);

	if (Settings.bRayTracing)
	{
		ComputeRayTracing(Camera, CmdList);
	}
	else
	{
		ClearSceneColor(Camera, CmdList);

		RenderGBufferPass(Camera, CmdList);

		RenderShadowDepths(Camera, CmdList);

		ComputeLightingPass(Camera, CmdList);

		CmdList.BeginRenderPass(Camera.SceneRP);

		RenderSkybox(Camera, CmdList);

		CmdList.EndRenderPass();
	}

	const uint32 ImageIndex = Surface.AcquireNextImage(Device);
	const gpu::Image& DisplayImage = Surface.GetImage(ImageIndex);
	ImageMemoryBarrier Barrier{ DisplayImage, EAccess::MemoryRead, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General };

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &Barrier);

	ComputePostProcessing(DisplayImage, Camera, CmdList);

	Barrier.srcAccessMask = EAccess::ShaderWrite;
	Barrier.dstAccessMask = EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite;
	Barrier.oldLayout = EImageLayout::General;
	Barrier.newLayout = EImageLayout::ColorAttachmentOptimal;

	CmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ColorAttachmentOutput, 0, nullptr, 1, &Barrier);

	ECS.GetSingletonComponent<ImGuiRenderData>().Render(Device, CmdList, Camera.UserInterfaceRP[ImageIndex]);

	Surface.Present(Device, ImageIndex, CmdList);

	Device.EndFrame();
}

void SceneRenderer::ClearSceneColor(CameraProxy& camera, gpu::CommandList& cmdList)
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

	imageBarrier.srcAccessMask = EAccess::TransferWrite;
	imageBarrier.dstAccessMask = EAccess::ShaderWrite;
	imageBarrier.oldLayout = EImageLayout::TransferDstOptimal;
	imageBarrier.newLayout = EImageLayout::General;

	cmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
}