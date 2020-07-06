#include "SceneRenderer.h"
#include <Engine/Engine.h>
#include <Components/RenderSettings.h>
#include <Systems/UserInterface.h>
#include "ShadowProxy.h"
#include "PostProcessing.h"

SceneRenderer::SceneRenderer(Engine& engine)
	: _Device(engine.Device)
	, _ShaderLibrary(engine.ShaderLibrary)
	, _Surface(engine.Surface)
	, _ECS(engine._ECS)
	, _Assets(engine.Assets)
	, _Camera(engine.Camera)
{
	_PostProcessingSet = _Device.CreateDescriptorSet<PostProcessingDescriptors>();
}

void SceneRenderer::Render(CameraProxy& camera)
{
	const RenderSettings& settings = _ECS.GetSingletonComponent<RenderSettings>();

	gpu::CommandList cmdList = _Device.CreateCommandList(EQueue::Graphics);

	if (settings.bRayTracing)
	{
		ComputeRayTracing(camera, cmdList);
	}
	else
	{
		ClearSceneColor(camera, cmdList);

		RenderGBufferPass(camera, cmdList);

		RenderShadowDepths(camera, cmdList);

		ComputeLightingPass(camera, cmdList);

		RenderSkybox(camera, cmdList);
	}

	const uint32 imageIndex = _Surface.AcquireNextImage(_Device);
	const gpu::Image& displayImage = _Surface.GetImage(imageIndex);
	ImageMemoryBarrier barrier{ displayImage, EAccess::MemoryRead, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General };

	cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &barrier);

	ComputePostProcessing(displayImage, camera, cmdList);

	barrier.srcAccessMask = EAccess::ShaderWrite;
	barrier.dstAccessMask = EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite;
	barrier.oldLayout = EImageLayout::General;
	barrier.newLayout = EImageLayout::ColorAttachmentOptimal;

	cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ColorAttachmentOutput, 0, nullptr, 1, &barrier);

	_ECS.GetSingletonComponent<ImGuiRenderData>().Render(_Device, cmdList, camera._UserInterfaceRP[imageIndex]);

	_Surface.Present(_Device, imageIndex, cmdList);

	_Device.EndFrame();
}

void SceneRenderer::ClearSceneColor(CameraProxy& camera, gpu::CommandList& cmdList)
{
	ImageMemoryBarrier imageBarrier
	{
		camera._SceneColor,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	};

	cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &imageBarrier);

	cmdList.ClearColorImage(camera._SceneColor, EImageLayout::TransferDstOptimal, {});

	imageBarrier.srcAccessMask = EAccess::TransferWrite;
	imageBarrier.dstAccessMask = EAccess::ShaderWrite;
	imageBarrier.oldLayout = EImageLayout::TransferDstOptimal;
	imageBarrier.newLayout = EImageLayout::General;

	cmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::ComputeShader, 0, nullptr, 1, &imageBarrier);
}