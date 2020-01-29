#include "SceneRenderer.h"
#include "FullscreenQuad.h"
#include "ShadowProxy.h"
#include <DRMShader.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Components/Transform.h>

SceneRenderer::SceneRenderer(DRMDevice& Device, drm::Surface& Surface, Scene& Scene, Screen& Screen)
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

		CreateDepthRP();
		CreateDepthVisualizationRP();
		CreateVoxelVisualizationRP();
		CreateLightingRP();
		CreateShadowMaskRP();
	});

	Cube = Scene.Assets.GetStaticMesh("Cube");

	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	VoxelColors = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage);
	VoxelPositions = Device.CreateBuffer(EBufferUsage::Storage, VoxelGridSize * VoxelGridSize * VoxelGridSize * sizeof(glm::ivec3));

	VoxelsDescriptorSet = Device.CreateDescriptorSet();
	VoxelsDescriptorSet->Write(VoxelColors, 1);
	VoxelsDescriptorSet->Write(VoxelPositions, 2);

	CreateVoxelRP();
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
		SceneTextures->Write(ShadowMask, SamplerState{ EFilter::Nearest }, 1);
		SceneTextures->Update();

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

		PipelineStateDesc PSODesc = {};
		PSODesc.Viewport.Width = SceneDepth->Width;
		PSODesc.Viewport.Height = SceneDepth->Height;
		PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSODesc.DepthStencilState.DepthWriteEnable = false;
		PSODesc.ShaderStages.Vertex = Scene.ShaderMap.FindShader<FullscreenVS>();
		PSODesc.ShaderStages.Fragment = Scene.ShaderMap.FindShader<FullscreenFS<EVisualize::Depth>>();

		CmdList.BindPipeline(PSODesc);

		CmdList.Draw(3, 1, 0, 0);

		CmdList.EndRenderPass();
	}
}

void SceneRenderer::Present(drm::CommandListRef CmdList)
{
	const uint32 ImageIndex = Surface.AcquireNextImage(Device);
	drm::ImageRef PresentImage = Surface.GetImage(ImageIndex);

	ImageMemoryBarrier Barrier(PresentImage, EAccess::MemoryRead, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal);

	CmdList->PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList->BlitImage(SceneColor, EImageLayout::TransferSrcOptimal, PresentImage, EImageLayout::TransferDstOptimal, EFilter::Nearest);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::MemoryRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::Present;

	CmdList->PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &Barrier);

	Surface.Present(Device, ImageIndex, CmdList);
}

void SceneRenderer::CreateDepthRP()
{
	RenderPassDesc RPDesc = { 0 };
	RPDesc.DepthAttachment = drm::AttachmentView(
		SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };
	DepthRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateVoxelRP()
{
	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	RenderPassDesc RPDesc = { 0 }; // Disable ROP
	RPDesc.RenderArea.Extent = glm::uvec2(VoxelGridSize);
	VoxelRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateVoxelVisualizationRP()
{
	RenderPassDesc RPDesc = { 1 };
	RPDesc.ColorAttachments[0] = drm::AttachmentView(
		SceneColor, 
		ELoadAction::DontCare, 
		EStoreAction::Store, 
		ClearColorValue{},
		EImageLayout::Undefined, 
		EImageLayout::TransferSrcOptimal);
	RPDesc.DepthAttachment = drm::AttachmentView(
		SceneDepth, 
		ELoadAction::Clear,
		EStoreAction::Store, 
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthWriteStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };
	VoxelVisualizationRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateLightingRP()
{
	RenderPassDesc RPDesc = { 1 };
	RPDesc.ColorAttachments[0] = drm::AttachmentView(SceneColor, ELoadAction::DontCare, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::TransferSrcOptimal);
	RPDesc.DepthAttachment = drm::AttachmentView(SceneDepth, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthReadStencilWrite, EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth->Width, SceneDepth->Height) };
	LightingRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateShadowMaskRP()
{
	RenderPassDesc RPDesc = { 1 };
	RPDesc.ColorAttachments[0] = drm::AttachmentView(
		ShadowMask, 
		ELoadAction::Clear, 
		EStoreAction::Store, 
		ClearColorValue{},
		EImageLayout::Undefined,
		EImageLayout::ShaderReadOnlyOptimal);
	RPDesc.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMask->Width, ShadowMask->Height) };
	ShadowMaskRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateDepthVisualizationRP()
{
	RenderPassDesc RPDesc = { 1 };
	RPDesc.ColorAttachments[0] = drm::AttachmentView(
		SceneColor, 
		ELoadAction::DontCare, 
		EStoreAction::Store, 
		ClearColorValue{}, 
		EImageLayout::Undefined,
		EImageLayout::TransferSrcOptimal);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneColor->Width, SceneColor->Height) };
	DepthVisualizationRP = Device.CreateRenderPass(RPDesc);
}