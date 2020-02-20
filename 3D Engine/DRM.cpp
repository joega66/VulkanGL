#include "DRM.h"

void drm::UploadImageData(DRMDevice& Device, const void* SrcPixels, const drm::Image& DstImage)
{
	drm::Buffer StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, DstImage.GetSize(), SrcPixels);

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	ImageMemoryBarrier Barrier(DstImage, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal);

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList.CopyBufferToImage(StagingBuffer, 0, DstImage, EImageLayout::TransferDstOptimal);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::ShaderRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &Barrier);

	Device.SubmitCommands(CmdList);
}