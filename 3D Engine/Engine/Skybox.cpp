#include "Skybox.h"

const std::string Skybox::_CubemapFaces[6] = { 
	"Left   [+X]", 
	"Right  [-X]", 
	"Up     [+Y]", 
	"Down   [-Y]", 
	"Front  [+Z]", 
	"Back   [-Z]" 
};

const std::string Skybox::_CubemapStems[6] = {
	"px",
	"nx",
	"py",
	"ny",
	"pz",
	"nz",
};

Skybox::Skybox(gpu::Device& device, const std::array<gpu::Image*, 6>& images, EFormat format)
	: _Images(images)
{
	_Image = device.CreateImage(_Images.front()->GetWidth(), _Images.front()->GetHeight(), 1, format, EImageUsage::Sampled | EImageUsage::Cubemap | EImageUsage::TransferDst);

	gpu::CommandBuffer cmdBuf = device.CreateCommandBuffer(EQueue::Transfer);

	std::vector<ImageMemoryBarrier> srcImageBarriers;
	srcImageBarriers.reserve(_Images.size());

	for (auto srcImage : _Images)
	{
		srcImageBarriers.push_back({
			*srcImage,
			EAccess::None,
			EAccess::TransferRead,
			EImageLayout::Undefined,
			EImageLayout::TransferSrcOptimal
		});
	}
	
	ImageMemoryBarrier dstImageBarrier{
		_Image,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	};

	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, srcImageBarriers.size(), srcImageBarriers.data());
	cmdBuf.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &dstImageBarrier);

	for (uint32 faceIndex = 0; faceIndex < _Images.size(); faceIndex++)
	{
		cmdBuf.CopyImage(
			*_Images[faceIndex],
			EImageLayout::TransferSrcOptimal, 
			_Image, 
			EImageLayout::TransferDstOptimal, 
			faceIndex
		);
	}

	for (auto& srcImageBarrier : srcImageBarriers)
	{
		srcImageBarrier.srcAccessMask = EAccess::TransferRead;
		srcImageBarrier.dstAccessMask = EAccess::MemoryRead;
		srcImageBarrier.oldLayout = EImageLayout::TransferSrcOptimal;
		srcImageBarrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	dstImageBarrier.srcAccessMask = EAccess::TransferWrite;
	dstImageBarrier.dstAccessMask = EAccess::MemoryRead;
	dstImageBarrier.oldLayout = EImageLayout::TransferDstOptimal;
	dstImageBarrier.newLayout = EImageLayout::ShaderReadOnlyOptimal;

	cmdBuf.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, srcImageBarriers.size(), srcImageBarriers.data());
	cmdBuf.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::TopOfPipe, 0, nullptr, 1, &dstImageBarrier);

	device.SubmitCommands(cmdBuf);
}