#include "Skybox.h"

const std::string Skybox::CubemapFaces[6] = { 
	"Left   [+X]", 
	"Right  [-X]", 
	"Up     [+Y]", 
	"Down   [-Y]", 
	"Front  [+Z]", 
	"Back   [-Z]" 
};

const std::string Skybox::CubemapStems[6] = {
	"px",
	"nx",
	"py",
	"ny",
	"pz",
	"nz",
};

Skybox::Skybox(gpu::Device& Device, const std::array<gpu::Image*, 6>& Images, EFormat Format)
	: Images(Images)
{
	Image = Device.CreateImage(Images.front()->GetWidth(), Images.front()->GetHeight(), 1, Format, EImageUsage::Sampled | EImageUsage::Cubemap | EImageUsage::TransferDst);

	gpu::CommandBuffer CmdList = Device.CreateCommandBuffer(EQueue::Transfer);

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

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, SrcImageBarriers.size(), SrcImageBarriers.data());
	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &DstImageBarrier);

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
		SrcImageBarrier.srcAccessMask = EAccess::TransferRead;
		SrcImageBarrier.dstAccessMask = EAccess::MemoryRead;
		SrcImageBarrier.oldLayout = EImageLayout::TransferSrcOptimal;
		SrcImageBarrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	DstImageBarrier.srcAccessMask = EAccess::TransferWrite;
	DstImageBarrier.dstAccessMask = EAccess::MemoryRead;
	DstImageBarrier.oldLayout = EImageLayout::TransferDstOptimal;
	DstImageBarrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, SrcImageBarriers.size(), SrcImageBarriers.data());
	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &DstImageBarrier);

	Device.SubmitCommands(CmdList);
}