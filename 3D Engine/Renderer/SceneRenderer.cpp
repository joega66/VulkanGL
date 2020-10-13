#include "SceneRenderer.h"
#include <Engine/Engine.h>
#include <Components/RenderSettings.h>
#include <Systems/UserInterface.h>
#include "ShadowProxy.h"
#include "PostProcessing.h"

SceneRenderer::SceneRenderer(Engine& engine)
	: _Device(engine._Device)
	, _ShaderLibrary(engine.ShaderLibrary)
	, _Compositor(engine._Compositor)
	, _ECS(engine._ECS)
	, _Assets(engine.Assets)
{
	_ScreenResizeEvent = engine._Screen.OnScreenResize([this] (int32 width, int32 height)
	{
		_Compositor.Resize(width, height, EImageUsage::Attachment | EImageUsage::Storage);

		const auto& images = _Compositor.GetImages();

		_UserInterfaceRP.clear();
		_UserInterfaceRP.reserve(images.size());

		for (const auto& image : images)
		{
			// After UI rendering, the image is ready for the present queue.
			RenderPassDesc rpDesc = {};
			rpDesc.colorAttachments.push_back(
				AttachmentView(&image, ELoadAction::Load, EStoreAction::Store, std::array<float, 4>{ 0.0f }, EImageLayout::ColorAttachmentOptimal, EImageLayout::Present));
			rpDesc.renderArea.extent = { image.GetWidth(), image.GetHeight() };
			rpDesc.srcStageMask = EPipelineStage::ComputeShader;
			rpDesc.dstStageMask = EPipelineStage::ColorAttachmentOutput;
			rpDesc.srcAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;
			rpDesc.dstAccessMask = EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite;

			_UserInterfaceRP.push_back(_Device.CreateRenderPass(rpDesc));
		}
	});

	_PostProcessingSet = _Device.CreateDescriptorSet<PostProcessingDescriptors>();
}

void SceneRenderer::Render()
{
	const RenderSettings& settings = _ECS.GetSingletonComponent<RenderSettings>();

	gpu::CommandList cmdList = _Device.CreateCommandList(EQueue::Graphics);

	auto& view = _ECS.GetComponent<Camera>(_ECS.GetEntities<Camera>().front());
	auto& camera = _ECS.GetComponent<CameraProxy>(_ECS.GetEntities<CameraProxy>().front());
	
	if (settings.bRayTracing)
	{
		ComputeRayTracing(view, camera, cmdList);
	}
	else
	{
		RenderGBufferPass(view, camera, cmdList);

		RenderShadowDepths(camera, cmdList);

		ComputeLightingPass(camera, cmdList);

		RenderSkybox(camera, cmdList);

		ComputeSSGI(view, camera, cmdList);
	}

	const uint32 imageIndex = _Compositor.AcquireNextImage();
	const gpu::Image& displayImage = _Compositor.GetImages()[imageIndex];
	ImageMemoryBarrier barrier{ displayImage, EAccess::MemoryRead, EAccess::ShaderWrite, EImageLayout::Undefined, EImageLayout::General };

	cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::ComputeShader, 0, nullptr, 1, &barrier);

	ComputePostProcessing(displayImage, camera, cmdList);

	barrier.srcAccessMask = EAccess::ShaderWrite;
	barrier.dstAccessMask = EAccess::ColorAttachmentRead | EAccess::ColorAttachmentWrite;
	barrier.oldLayout = EImageLayout::General;
	barrier.newLayout = EImageLayout::ColorAttachmentOptimal;

	cmdList.PipelineBarrier(EPipelineStage::ComputeShader, EPipelineStage::ColorAttachmentOutput, 0, nullptr, 1, &barrier);

	_ECS.GetSingletonComponent<ImGuiRenderData>().Render(_Device, cmdList, _UserInterfaceRP[imageIndex]);

	_Compositor.Present(imageIndex, cmdList);

	_Device.EndFrame();
}