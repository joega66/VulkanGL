#include "GlobalRenderResources.h"
#include <Engine/Engine.h>
#include <Engine/Screen.h>

GlobalRenderResources::GlobalRenderResources(Engine& Engine)
	: Device(Engine.Device)
	, Surface(Engine.Surface)
	, CameraDescriptorSet(Engine.Device)
	, SkyboxDescriptorSet(Engine.Device)
	, SceneTexturesDescriptorSet(Engine.Device)
	, VCTLightingCache(Engine)
{
	class VCTLightingCache* VCTLightingCachePtr = &VCTLightingCache;
	Engine._Screen.ScreenResizeEvent([this, VCTLightingCachePtr] (int32 Width, int32 Height)
	{
		Surface.Resize(Device, Width, Height);

		const drm::Image& SwapchainImage = Surface.GetImage(0);
		SceneColor = Device.CreateImage(Width, Height, 1, SwapchainImage.GetFormat(), EImageUsage::Attachment | EImageUsage::TransferSrc);
		SceneDepth = Device.CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

		SceneTexturesDescriptorSet.Depth = drm::ImageView(SceneDepth, Device.CreateSampler({ EFilter::Nearest }));
		SceneTexturesDescriptorSet.Update();

		CreateDepthRP(Device);
		CreateDepthVisualizationRP(Device);
		CreateLightingRP(Device);

		if (VCTLightingCachePtr->IsDebuggingEnabled())
		{
			VCTLightingCachePtr->CreateDebugRenderPass(SceneColor, SceneDepth);
		}
	});
}

void GlobalRenderResources::CreateDepthRP(DRMDevice& Device)
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

void GlobalRenderResources::CreateLightingRP(DRMDevice& Device)
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(&SceneColor, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::TransferSrcOptimal));
	RPDesc.DepthAttachment = drm::AttachmentView(&SceneDepth, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthReadStencilWrite, EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	LightingRP = Device.CreateRenderPass(RPDesc);
}

void GlobalRenderResources::CreateDepthVisualizationRP(DRMDevice& Device)
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