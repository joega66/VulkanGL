#include "Skybox.h"

Skybox::Skybox(DRMDevice& Device, const std::array<std::string, 6>& Files, EFormat Format)
{
	drm::Buffer StagingBuffer;
	void* MemMapped = nullptr;

	for (uint32 FaceIndex = 0; FaceIndex < Files.size(); FaceIndex++)
	{
		int32 Width, Height, Channels;
		uint8* Pixels = Platform::LoadImage(Files[FaceIndex], Width, Height, Channels);

		if (FaceIndex == 0)
		{
			Image = Device.CreateImage(Width, Height, 1, Format, EImageUsage::Sampled | EImageUsage::Cubemap | EImageUsage::TransferDst);
			StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Image.GetSize());
			MemMapped = StagingBuffer.GetData();
		}

		check(Image.GetWidth() == Width && Image.GetHeight() == Height, "Cubemap faces must all be the same size.");

		Platform::Memcpy(static_cast<uint8*>(MemMapped) + FaceIndex * static_cast<uint64>(Image.GetSize() / 6), Pixels, (Image.GetSize() / 6));

		Platform::FreeImage(Pixels);
	}

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	ImageMemoryBarrier Barrier{
		Image,
		EAccess::None,
		EAccess::TransferWrite,
		EImageLayout::Undefined,
		EImageLayout::TransferDstOptimal
	};

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, 1, &Barrier);

	CmdList.CopyBufferToImage(StagingBuffer, 0, Image, EImageLayout::TransferDstOptimal);

	Barrier.SrcAccessMask = EAccess::TransferWrite;
	Barrier.DstAccessMask = EAccess::ShaderRead;
	Barrier.OldLayout = EImageLayout::TransferDstOptimal;
	Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, 1, &Barrier);

	Device.SubmitCommands(CmdList);
}