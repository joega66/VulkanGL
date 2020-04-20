#include "Skybox.h"

Skybox::Skybox(DRMDevice& Device, const std::array<const drm::Image*, 6>& Images, EFormat Format)
	: Images(Images)
{
	Image = Device.CreateImage(Images.front()->GetWidth(), Images.front()->GetHeight(), 1, Format, EImageUsage::Sampled | EImageUsage::Cubemap | EImageUsage::TransferDst);

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	std::vector<ImageMemoryBarrier> SrcImageBarriers;
	SrcImageBarriers.reserve(Images.size());

	for (auto SrcImage : Images)
	{
		SrcImageBarriers.push_back({
			*SrcImage,
			EAccess::None,
			EAccess::TransferRead,
			EImageLayout::Undefined,
			EImageLayout::TransferSrcOptimal
		});
	}
	
	ImageMemoryBarrier DstImageBarrier{
		Image,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	};

	CmdList.PipelineBarrier(EPipelineStage::Host, EPipelineStage::Transfer, 0, nullptr, SrcImageBarriers.size(), SrcImageBarriers.data());
	CmdList.PipelineBarrier(EPipelineStage::Host, EPipelineStage::Transfer, 0, nullptr, 1, &DstImageBarrier);

	for (uint32 FaceIndex = 0; FaceIndex < Images.size(); FaceIndex++)
	{
		CmdList.CopyImage(
			*Images[FaceIndex], 
			EImageLayout::TransferSrcOptimal, 
			Image, 
			EImageLayout::TransferDstOptimal, 
			FaceIndex
		);
	}

	for (auto& SrcImageBarrier : SrcImageBarriers)
	{
		SrcImageBarrier.SrcAccessMask = EAccess::TransferRead;
		SrcImageBarrier.DstAccessMask = EAccess::ShaderRead;
		SrcImageBarrier.OldLayout = EImageLayout::TransferSrcOptimal;
		SrcImageBarrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	DstImageBarrier.SrcAccessMask = EAccess::TransferWrite;
	DstImageBarrier.DstAccessMask = EAccess::ShaderRead;
	DstImageBarrier.OldLayout = EImageLayout::TransferDstOptimal;
	DstImageBarrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, SrcImageBarriers.size(), SrcImageBarriers.data());
	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &DstImageBarrier);

	Device.SubmitCommands(CmdList);
}