#include "CameraProxy.h"
#include <Components/Camera.h>

CameraProxy::CameraProxy(gpu::Device& device)
{	
}

void CameraProxy::Resize(gpu::Device& device, uint32 width, uint32 height)
{
	_DirectLighting = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Attachment | EImageUsage::Storage);

	_SceneColor = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Attachment | EImageUsage::Storage | EImageUsage::TransferDst);
	_SceneDepth = device.CreateImage(width, height, 1, EFormat::D32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);

	_GBuffer0 = device.CreateImage(width, height, 1, EFormat::R32G32B32A32_SFLOAT, EImageUsage::Attachment | EImageUsage::Sampled);
	_GBuffer1 = device.CreateImage(width, height, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Attachment | EImageUsage::Sampled);

	_SSRHistory = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Storage);
	_SSGIHistory = device.CreateImage(width, height, 1, EFormat::R16G16B16A16_SFLOAT, EImageUsage::Storage);

	CreateGBufferRP(device);

	CreateSkyboxRP(device);

	gpu::CommandBuffer cmdBuf = device.CreateCommandBuffer(EQueue::Transfer);

	ImageMemoryBarrier barriers[] = { { _SSRHistory }, { _SSGIHistory } };

	for (auto& barrier : barriers)
	{
		barrier.srcAccessMask = EAccess::None;
		barrier.dstAccessMask = EAccess::MemoryRead | EAccess::MemoryWrite;
		barrier.oldLayout = EImageLayout::Undefined;
		barrier.newLayout = EImageLayout::General;
	}

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::TopOfPipe, 0, nullptr, std::size(barriers), barriers);

	device.SubmitCommands(cmdBuf);
}

void CameraProxy::CreateGBufferRP(gpu::Device& device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments =
	{
		AttachmentView(&_GBuffer0, ELoadAction::Clear, EStoreAction::Store, std::array<float, 4>{ 0.0f }, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal),
		AttachmentView(&_GBuffer1, ELoadAction::Clear, EStoreAction::Store, std::array<float, 4>{ 0.0f }, EImageLayout::Undefined, EImageLayout::ShaderReadOnlyOptimal)
	};
	rpDesc.depthAttachment = AttachmentView(
		&_SceneDepth,
		ELoadAction::Clear,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::Undefined,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(_SceneDepth.GetWidth(), _SceneDepth.GetHeight()) };
	rpDesc.srcStageMask = EPipelineStage::TopOfPipe;
	rpDesc.dstStageMask = EPipelineStage::ComputeShader;
	rpDesc.srcAccessMask = EAccess::None;
	rpDesc.dstAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;

	_GBufferRP = device.CreateRenderPass(rpDesc);
}

void CameraProxy::CreateSkyboxRP(gpu::Device& device)
{
	RenderPassDesc rpDesc = {};
	rpDesc.colorAttachments.push_back(
		AttachmentView(&_DirectLighting, ELoadAction::Load, EStoreAction::Store, std::array<float, 4>{ 0.0f }, EImageLayout::General, EImageLayout::General));
	rpDesc.depthAttachment = AttachmentView(
		&_SceneDepth,
		ELoadAction::Load,
		EStoreAction::Store,
		ClearDepthStencilValue{},
		EImageLayout::DepthReadStencilWrite,
		EImageLayout::DepthReadStencilWrite);
	rpDesc.renderArea = RenderArea{ glm::ivec2(), glm::uvec2(_SceneDepth.GetWidth(), _SceneDepth.GetHeight()) };
	rpDesc.srcStageMask = EPipelineStage::ComputeShader;
	rpDesc.dstStageMask = EPipelineStage::ComputeShader;
	rpDesc.srcAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;
	rpDesc.dstAccessMask = EAccess::ShaderRead | EAccess::ShaderWrite;

	_SkyboxRP = device.CreateRenderPass(rpDesc);
}