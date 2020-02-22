#include "SceneRenderer.h"
#include <DRMShader.h>
#include <Engine/Engine.h>
#include <Engine/Scene.h>
#include <Engine/AssetManager.h>
#include <Engine/Screen.h>
#include <Components/Transform.h>
#include <Systems/UserInterface.h>
#include "FullscreenQuad.h"
#include "ShadowProxy.h"

SceneRenderer::SceneRenderer(Engine& Engine)
	: Device(Engine.Device)
	, ShaderMap(Engine.ShaderMap)
	, Surface(Engine.Surface)
	, SceneTextures(Engine.Device)
	, VoxelDescriptorSet(Engine.Device)
{
	Engine._Screen.ScreenResizeEvent([&](int32 Width, int32 Height)
	{
		Surface.Resize(Device, Width, Height);

		const drm::Image& SwapchainImage = Surface.GetImage(0);
		SceneColor = Device.CreateImage(Width, Height, 1, SwapchainImage.GetFormat(), EImageUsage::Attachment | EImageUsage::TransferSrc);

		SceneDepth = Device.CreateImage(Width, Height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
		SceneTextures.Depth = &SceneDepth;
		SceneTextures.DepthSampler = Device.CreateSampler({ EFilter::Nearest });
	
		ShadowMask = Device.CreateImage(Width, Height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);
		SceneTextures.ShadowMaskSampler = Device.CreateSampler({ EFilter::Nearest });

		CreateDepthRP();
		CreateDepthVisualizationRP();
		CreateVoxelVisualizationRP();
		CreateLightingRP();
		CreateShadowMaskRP();
	});

	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	check(VoxelGridSize <= 1024, "Exceeded voxel bits.");
	VoxelColors = Device.CreateImage(VoxelGridSize, VoxelGridSize, VoxelGridSize, EFormat::R8G8B8A8_UNORM, EImageUsage::Storage);
	VoxelDescriptorSet.VoxelColors = &VoxelColors;
	VoxelPositions = Device.CreateBuffer(EBufferUsage::Storage, VoxelGridSize * VoxelGridSize * VoxelGridSize * sizeof(int32));
	VoxelDescriptorSet.VoxelPositions = &VoxelPositions;

	CreateVoxelRP();

	Cube = Engine.Assets.GetStaticMesh("Cube");
}

void SceneRenderer::Render(UserInterface& UserInterface, SceneProxy& Scene)
{
	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Graphics);

	if (Platform::GetBool("Engine.ini", "Voxels", "RenderVoxels", false))
	{
		RenderVoxels(Scene, CmdList);
	}
	else
	{
		SceneTextures.ShadowMask = &ShadowMask;
		SceneTextures.Update();

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

			UserInterface.Render(Device, LightingRP, CmdList);

			CmdList.EndRenderPass();
		}
	}

	Present(CmdList);

	Device.EndFrame();
}

void SceneRenderer::RenderDepthVisualization(SceneProxy& Scene, drm::CommandList& CmdList)
{
	for (auto Entity : Scene.ECS.GetEntities<ShadowProxy>())
	{
		const auto& ShadowProxy = Scene.ECS.GetComponent<class ShadowProxy>(Entity);

		PipelineStateDesc PSODesc = {};
		PSODesc.RenderPass = &DepthVisualizationRP;
		PSODesc.Viewport.Width = SceneDepth.GetWidth();
		PSODesc.Viewport.Height = SceneDepth.GetHeight();
		PSODesc.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
		PSODesc.DepthStencilState.DepthWriteEnable = false;
		PSODesc.ShaderStages.Vertex = ShaderMap.FindShader<FullscreenVS>();
		PSODesc.ShaderStages.Fragment = ShaderMap.FindShader<FullscreenFS<EVisualize::Depth>>();
		PSODesc.DescriptorSets = { &ShadowProxy.GetDescriptorSet() };

		drm::Pipeline DepthVisualizationPipeline = Device.CreatePipeline(PSODesc);

		CmdList.BeginRenderPass(DepthVisualizationRP);

		CmdList.BindPipeline(DepthVisualizationPipeline);

		CmdList.BindDescriptorSets(DepthVisualizationPipeline, PSODesc.DescriptorSets.size(), PSODesc.DescriptorSets.data());

		CmdList.Draw(3, 1, 0, 0);

		CmdList.EndRenderPass();
	}
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

void SceneRenderer::CreateVoxelRP()
{
	const uint32 VoxelGridSize = Platform::GetInt("Engine.ini", "Voxels", "VoxelGridSize", 256);
	RenderPassDesc RPDesc = {}; // Disable ROP
	RPDesc.RenderArea.Extent = glm::uvec2(VoxelGridSize);
	VoxelRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateVoxelVisualizationRP()
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(
		&SceneColor, 
		ELoadAction::DontCare, 
		EStoreAction::Store,
		ClearColorValue{},
		EImageLayout::Undefined, 
		EImageLayout::TransferSrcOptimal)
	);
	RPDesc.DepthAttachment = drm::AttachmentView(
		&SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store, 
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthWriteStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	VoxelVisualizationRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateLightingRP()
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(&SceneColor, ELoadAction::Clear, EStoreAction::Store, ClearColorValue{}, EImageLayout::Undefined, EImageLayout::TransferSrcOptimal));
	RPDesc.DepthAttachment = drm::AttachmentView(&SceneDepth, ELoadAction::Load, EStoreAction::DontCare, ClearDepthStencilValue{}, EImageLayout::DepthReadStencilWrite, EImageLayout::DepthReadStencilWrite);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneDepth.GetWidth(), SceneDepth.GetHeight()) };
	LightingRP = Device.CreateRenderPass(RPDesc);
}

void SceneRenderer::CreateShadowMaskRP()
{
	RenderPassDesc RPDesc = {};
	RPDesc.ColorAttachments.push_back(drm::AttachmentView(
		&ShadowMask,
		ELoadAction::Clear, 
		EStoreAction::Store, 
		ClearColorValue{},
		EImageLayout::Undefined,
		EImageLayout::ShaderReadOnlyOptimal)
	);
	RPDesc.RenderArea = RenderArea{ glm::ivec2{}, glm::uvec2(ShadowMask.GetWidth(), ShadowMask.GetHeight()) };
	ShadowMaskRP = Device.CreateRenderPass(RPDesc);
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
		EImageLayout::TransferSrcOptimal)
	);
	RPDesc.RenderArea = RenderArea{ glm::ivec2(), glm::uvec2(SceneColor.GetWidth(), SceneColor.GetHeight()) };
	DepthVisualizationRP = Device.CreateRenderPass(RPDesc);
}