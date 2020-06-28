#include "DRM.h"

void drm::UploadImageData(drm::Device& device, const void* srcPixels, const drm::Image& dstImage)
{
	drm::Buffer stagingBuffer = device.CreateBuffer(EBufferUsage::Transfer, dstImage.GetSize(), srcPixels);

	drm::CommandList cmdList = device.CreateCommandList(EQueue::Transfer);

	ImageMemoryBarrier barrier{ dstImage, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal };

	cmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &barrier);

	cmdList.CopyBufferToImage(stagingBuffer, 0, dstImage, EImageLayout::TransferDstOptimal);

	barrier.srcAccessMask = EAccess::TransferWrite;
	barrier.dstAccessMask = EAccess::ShaderRead;
	barrier.oldLayout = EImageLayout::TransferDstOptimal;
	barrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;

	cmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &barrier);

	device.SubmitCommands(cmdList);
}