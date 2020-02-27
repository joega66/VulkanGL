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

SceneRenderer::SceneRenderer(Engine& Engine)
	: Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
	, Surface(Engine.Surface)
	, ECS(Engine.ECS)
	, CameraDescriptorSet(Engine.Device)
	, SkyboxDescriptorSet(Engine.Device)
	, SceneTexturesDescriptorSet(Engine.Device)
	, VCTLightingCache(Engine)
{
	class VCTLightingCache* VCTLightingCachePtr = &VCTLightingCache;
	Engine._Screen.ScreenResizeEvent([this, VCTLightingCachePtr](int32 Width, int32 Height)
	{
		Surface.Resize(Device, Width, Height);

		const drm::Image& SwapchainImage = Surface.GetImage(0);
		SceneColor = Device.CreateImage(Width, Height, 1, SwapchainImage.GetFormat(), EImageUsage::Attachment | EImageUsage::TransferSrc);
		SceneDepth = Device.CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

		SceneTexturesDescriptorSet.Depth = drm::ImageView(SceneDepth, Device.CreateSampler({ EFilter::Nearest }));
		SceneTexturesDescriptorSet.Update();

		CreateDepthRP();
		CreateDepthVisualizationRP();
		CreateLightingRP();

		if (VCTLightingCachePtr->IsDebuggingEnabled())
		{
			VCTLightingCachePtr->CreateDebugRenderPass(SceneColor, SceneDepth);
		}
	});

	Cube = Engine.Assets.GetStaticMesh("Cube");
}

void SceneRenderer::Render(UserInterface& UserInterface, SceneProxy& Scene)
{
	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Graphics);

	RenderDepthPrepass(Scene, CmdList);

	RenderShadowDepths(Scene, CmdList);

	VCTLightingCache.Render(*this, Scene, CmdList);

	if (ECS.GetSingletonComponent<RenderSettings>().DrawVoxels && VCTLightingCache.IsDebuggingEnabled())
	{
		VCTLightingCache.RenderVisualization(*this, CmdList);
	}
	else
	{
		RenderLightingPass(Scene, CmdList);
	}
	
	RenderSkybox(Scene, CmdList);

	UserInterface.Render(Device, LightingRP, CmdList);

	CmdList.EndRenderPass();

	Present(CmdList);

	Device.EndFrame();
}

void SceneRenderer::Present(drm::CommandList& CmdList)
{
	const uint32 ImageIndex = Surface.AcquireNextImage(Device);
	const drm::Image& PresentImage = Surface.GetImage(ImageIndex);

	ImageMemoryBarrier Barrier(PresentImage, EAccess::MemoryRead, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList.BlitImage(SceneColor, EImageLayout::TransferSrcOptimal, PresentImage, EImageLayout::TransferDstOptimal, EFilter::Nearest);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::MemoryRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::Present;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &Barrier);

	Surface.Present(Device, ImageIndex, CmdList);
}

void SceneRenderer::CreateDepthRP()
{
	RenderPassDesc RPDesc = {};
	RPDesc.DepthAttachment = drm::AttachmentView(
		&SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	DepthRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateLightingRP()
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(&SceneColor, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::TransferSrcOptimal));
	RPDesc.DepthAttachment = drm::AttachmentView(&SceneDepth, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthReadStencilWrite, EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	LightingRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateDepthVisualizationRP()
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(
		&SceneColor, 
		ELoadAction::DontCare, 
		EStoreAction::Store, 
		ClearColorValue{}, 
		EImageLayout::Undefined,
		EImageLayout::TransferSrcOptimal));
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneColor.GetWidth(), SceneColor.GetHeight()) };
	DepthVisualizationRP = Device.CreateRenderPass(RPDesc);
}