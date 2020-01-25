#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include <DRMShader.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>

SceneRenderer::SceneRenderer(DRM& Device, drm::Surface& Surface, Scene& Scene, Screen& Screen)
	: Device(Device)
	, Surface(Surface)
{
	SceneTextures = Device.CreateDescriptorSet();

	Screen.ScreenResizeEvent([&](int32 Width, int32 Height)
	{
		Surface.Resize(Device, Width, Height);

		drm::ImageRef SwapchainImage = Surface.GetImage(0);
		SceneColor = Device.CreateImage(Width, Height, 1, SwapchainImage->Format, EImageUsage::Attachment | EImageUsage::TransferSrc);

		SceneDepth = Device.CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		ShadowMask = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);

		SceneTextures->Write(SceneDepth, SamplerState{ EFilter::Nearest }, 0);
		SceneTextures->Write(ShadowMask, SamplerState{ EFilter::Nearest }, 1);
		SceneTextures->Update();

		CreateDepthRP(Device);
		CreateDepthVisualizationRP(Device);
		CreateVoxelVisualizationRP(Device);
		CreateLightingRP(Device);
		CreateShadowMaskRP(Device);
	});

	Cube = Scene.Assets.GetStaticMesh("Cube");

	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	VoxelColors = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage, EImageLayout::General);
	VoxelPositions = Device.CreateBuffer(EBufferUsage::Storage, VoxelGridSize * VoxelGridSize * VoxelGridSize * sizeof(glm::ivec3));

	VoxelsDescriptorSet = Device.CreateDescriptorSet();
	VoxelsDescriptorSet->Write(VoxelColors, 1);
	VoxelsDescriptorSet->Write(VoxelPositions, 2);

	CreateVoxelRP(Device);

	ShadowProxy::InitCallbacks(Device, Scene.ECS);
}

void SceneRenderer::Render(SceneProxy& Scene)
{
	drm::CommandListRef CommandList = Device.CreateCommandList();
	drm::CommandList& CmdList = *CommandList;

	if (Platform::GetBool("Engine.ini", "Voxels", "RenderVoxels", false))
	{
		RenderVoxels(Scene, CmdList);
	}
	else
	{
		RenderDepthPrepass(Scene, CmdList);

		RenderShadowDepths(Scene, CmdList);

		if (Platform::GetBool("Engine.ini", "Shadows", "Visualize", false))
		{
			RenderDepthVisualization(Scene, CmdList);
		}
		else
		{
			CmdList.BeginRenderPass(LightingRP);

			RenderLightingPass(Scene, CmdList);

			RenderSkybox(Scene, CmdList);

			CmdList.EndRenderPass();
		}
	}

	Present(CommandList);

	Device.EndFrame();
}

void SceneRenderer::RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		auto& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);
		auto ShadowMap = ShadowProxy.GetShadowMap();

		CmdList.BeginRenderPass(DepthVisualizationRP);

		drm::DescriptorSetRef DescriptorSet = Device.CreateDescriptorSet();
		DescriptorSet->Write(ShadowMap, SamplerState{ EFilter::Nearest }, 0);
		DescriptorSet->Update();

		CmdList.BindDescriptorSets(1, &DescriptorSet);

		PipelineStateInitializer PSOInit = {};
		PSOInit.Viewport.Width = SceneDepth->Width;
		PSOInit.Viewport.Height = SceneDepth->Height;
		PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSOInit.DepthStencilState.DepthWriteEnable = false;
		PSOInit.ShaderStages.Vertex = Scene.ShaderMap.FindShader<FullscreenVS>();
		PSOInit.ShaderStages.Fragment = Scene.ShaderMap.FindShader<FullscreenFS<EVisualize::Depth>>();

		CmdList.BindPipeline(PSOInit);

		CmdList.Draw(3, 1, 0, 0);

		CmdList.EndRenderPass();
	}
}

void SceneRenderer::Present(drm::CommandListRef CmdList)
{
	const uint32 ImageIndex = Surface.AcquireNextImage(Device);
	drm::ImageRef PresentImage = Surface.GetImage(ImageIndex);

	ImageMemoryBarrier Barrier(PresentImage, EAccess::MemoryRead, EAccess::TransferWrite, EImageLayout::Present, EImageLayout::TransferDstOptimal);

	CmdList->PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList->BlitImage(SceneColor, EImageLayout::TransferSrcOptimal, PresentImage, EImageLayout::TransferDstOptimal, EFilter::Nearest);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::MemoryRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::Present;

	CmdList->PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &Barrier);

	Surface.Present(Device, ImageIndex, CmdList);
}

void SceneRenderer::CreateDepthRP(DRM& Device)
{
	RenderPassInitializer DepthRPInit = { 0 };
	DepthRPInit.DepthAttachment = drm::AttachmentView(
		SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	DepthRPInit.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };
	DepthRP = Device.CreateRenderPass(DepthRPInit);
}

void SceneRenderer::CreateVoxelRP(DRM& Device)
{
	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	RenderPassInitializer RPInfo = { 0 }; // Disable ROP
	RPInfo.RenderArea.Extent = glm::uvec2(VoxelGridSize);
	VoxelRP = Device.CreateRenderPass(RPInfo);
}

void SceneRenderer::CreateVoxelVisualizationRP(DRM& Device)
{
	RenderPassInitializer RPInfo = { 1 };
	RPInfo.ColorAttachments[0] = drm::AttachmentView(
		SceneColor, 
		ELoadAction::DontCare, 
		EStoreAction::Store, 
		ClearColorValue{},
		EImageLayout::Undefined, 
		EImageLayout::TransferSrcOptimal);
	RPInfo.DepthAttachment = drm::AttachmentView(
		SceneDepth, 
		ELoadAction::Clear,
		EStoreAction::Store, 
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthWriteStencilWrite);
	RPInfo.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };
	VoxelVisualizationRP = Device.CreateRenderPass(RPInfo);
}

void SceneRenderer::CreateLightingRP(DRM& Device)
{
	RenderPassInitializer RPInfo = { 1 };
	RPInfo.ColorAttachments[0] = drm::AttachmentView(SceneColor, ELoadAction::DontCare, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::TransferSrcOptimal);
	RPInfo.DepthAttachment = drm::AttachmentView(SceneDepth, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthReadStencilWrite, EImageLayout::DepthReadStencilWrite);
	RPInfo.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };
	LightingRP = Device.CreateRenderPass(RPInfo);
}

void SceneRenderer::CreateShadowMaskRP(DRM& Device)
{
	RenderPassInitializer RPInfo = { 1 };
	RPInfo.ColorAttachments[0] = drm::AttachmentView(
		ShadowMask, 
		ELoadAction::Clear, 
		EStoreAction::Store, 
		ClearColorValue{},
		EImageLayout::Undefined,
		EImageLayout::ShaderReadOnlyOptimal);
	RPInfo.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMask->Width, ShadowMask->Height) };
	ShadowMaskRP = Device.CreateRenderPass(RPInfo);
}

void SceneRenderer::CreateDepthVisualizationRP(DRM& Device)
{
	RenderPassInitializer RPInfo = { 1 };
	RPInfo.ColorAttachments[0] = drm::AttachmentView(
		SceneColor, 
		ELoadAction::DontCare, 
		EStoreAction::Store, 
		ClearColorValue{}, 
		EImageLayout::Undefined,
		EImageLayout::TransferSrcOptimal);
	RPInfo.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneColor->Width, SceneColor->Height) };
	DepthVisualizationRP = Device.CreateRenderPass(RPInfo);
}