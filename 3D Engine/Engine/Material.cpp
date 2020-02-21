#include "Material.h"
#include <DRM.h>

drm::Image Material::Red;
drm::Image Material::Green;
drm::Image Material::Blue;
drm::Image Material::White;
drm::Image Material::Black;
drm::Image Material::Dummy;

Material::Material(
	DRMDevice& Device,
	const MaterialDescriptors& InDescriptors,
	EMaterialMode MaterialMode,
	float Roughness,
	float Metallicity)
	: Descriptors(InDescriptors)
	, MaterialMode(MaterialMode)
	, Roughness(Roughness)
	, Metallicity(Metallicity)
{
	struct PBRUniformData
	{
		float Roughness;
		float Metallicity;
	} PBRUniformData = { Roughness, Metallicity };

	PBRUniform = Device.CreateBuffer(EBufferUsage::Uniform | EBufferUsage::HostVisible, sizeof(PBRUniformData), &PBRUniformData);
	Descriptors.PBRUniform = &PBRUniform;
	Descriptors.BaseColorSampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear });
	Descriptors.MetallicRoughnessSampler = Device.CreateSampler({ EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear });

	SpecializationInfo.Add(0, Descriptors.MetallicRoughness != &Material::Dummy);
	SpecializationInfo.Add(1, MaterialMode == EMaterialMode::Masked);
}

void Material::CreateDebugMaterials(DRMDevice& Device)
{
	std::vector<uint8> Colors =
	{
		255, 0, 0, 0, // Red
		0, 255, 0, 0, // Green
		0, 0, 255, 0, // Blue
		255, 255, 255, 0, // White
		0, 0, 0, 0, // Black
		234, 115, 79, 0 // Dummy
	};

	drm::Buffer StagingBuffer = Device.CreateBuffer(EBufferUsage::Transfer, Colors.size(), Colors.data());

	Material::Red = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Green = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Blue = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::White = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Black = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);
	Material::Dummy = Device.CreateImage(1, 1, 1, EFormat::R8G8B8A8_UNORM, EImageUsage::Sampled | EImageUsage::TransferDst);

	drm::CommandList CmdList = Device.CreateCommandList(EQueue::Transfer);

	std::vector<ImageMemoryBarrier> Barriers
	{
		ImageMemoryBarrier(Material::Red, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal),
		ImageMemoryBarrier(Material::Green, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal),
		ImageMemoryBarrier(Material::Blue, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal),
		ImageMemoryBarrier(Material::White, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal),
		ImageMemoryBarrier(Material::Black, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal),
		ImageMemoryBarrier(Material::Dummy, EAccess::None, EAccess::TransferWrite, EImageLayout::Undefined, EImageLayout::TransferDstOptimal)
	};

	CmdList.PipelineBarrier(EPipelineStage::TopOfPipe, EPipelineStage::Transfer, 0, nullptr, Barriers.size(), Barriers.data());

	for (uint32 ColorIndex = 0; ColorIndex < Barriers.size(); ColorIndex++)
	{
		CmdList.CopyBufferToImage(StagingBuffer, ColorIndex * 4 * sizeof(uint8), Barriers[ColorIndex].Image, EImageLayout::TransferDstOptimal);
	}

	for (auto& Barrier : Barriers)
	{
		Barrier.SrcAccessMask = EAccess::TransferWrite;
		Barrier.DstAccessMask = EAccess::ShaderRead;
		Barrier.OldLayout = EImageLayout::TransferDstOptimal;
		Barrier.NewLayout = EImageLayout::ShaderReadOnlyOptimal;
	}

	CmdList.PipelineBarrier(EPipelineStage::Transfer, EPipelineStage::FragmentShader, 0, nullptr, Barriers.size(), Barriers.data());

	Device.SubmitCommands(CmdList);
}

MaterialDescriptors::MaterialDescriptors()
	: BaseColor(&Material::Dummy)
	, MetallicRoughness(&Material::Dummy)
{
}
