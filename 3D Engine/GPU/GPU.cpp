#include "GPU/GPU.h"

void gpu::UploadImageData(gpu::Device& device, const void* srcPixels, const gpu::Image& dstImage)
{
	gpu::CommandBuffer cmdBuf = device.CreateCommandBuffer(EQueue::Transfer);

	auto stagingBuffer = cmdBuf.CreateStagingBuffer(dstImage.GetSize(), srcPixels);

	ImageMemoryBarrier barrier{ dstImage, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal };

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &barrier);

	cmdBuf.CopyBufferToImage(*stagingBuffer, 0, dstImage, EImageLayout::TransferDstOptimal);

	barrier.srcAccessMask = EAccess::TransferWrite;
	barrier.dstAccessMask = EAccess::MemoryRead;
	barrier.oldLayout = EImageLayout::TransferDstOptimal;
	barrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;

	cmdBuf.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &barrier);

	device.SubmitCommands(cmdBuf);
}
