#include "VulkanImage.h"
#include "VulkanDevice.h"

static const HashTable<EFormat, VkFormat> VulkanFormat =
{
	ENTRY(EFormat::UNDEFINED, VK_FORMAT_UNDEFINED)
	ENTRY(EFormat::R8_UNORM, VK_FORMAT_R8_UNORM)
	ENTRY(EFormat::R8_UINT, VK_FORMAT_R8_UINT)
	ENTRY(EFormat::R8_SRGB, VK_FORMAT_R8_SRGB)
	ENTRY(EFormat::R8G8_UINT, VK_FORMAT_R8G8_UINT)
	ENTRY(EFormat::R8G8_SINT, VK_FORMAT_R8G8_SINT)
	ENTRY(EFormat::R8G8_SRGB, VK_FORMAT_R8G8_SRGB)
	ENTRY(EFormat::R8G8B8_UINT, VK_FORMAT_R8G8B8_UINT)
	ENTRY(EFormat::R8G8B8_SINT, VK_FORMAT_R8G8B8_SINT)
	ENTRY(EFormat::R8G8B8_SRGB, VK_FORMAT_R8G8B8_SRGB)
	ENTRY(EFormat::B8G8R8_UINT, VK_FORMAT_B8G8R8_UINT)
	ENTRY(EFormat::B8G8R8_SINT, VK_FORMAT_B8G8R8_SINT)
	ENTRY(EFormat::B8G8R8_SRGB, VK_FORMAT_B8G8R8_SRGB)
	ENTRY(EFormat::R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT)
	ENTRY(EFormat::R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT)
	ENTRY(EFormat::R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB)
	ENTRY(EFormat::R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM)
	ENTRY(EFormat::B8G8R8A8_UINT, VK_FORMAT_B8G8R8A8_UINT)
	ENTRY(EFormat::B8G8R8A8_SINT, VK_FORMAT_B8G8R8A8_SINT)
	ENTRY(EFormat::B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB)
	ENTRY(EFormat::B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)
	ENTRY(EFormat::R16_UNORM, VK_FORMAT_R16_UNORM)
	ENTRY(EFormat::R16_UINT, VK_FORMAT_R16_UINT)
	ENTRY(EFormat::R16_SINT, VK_FORMAT_R16_SINT)
	ENTRY(EFormat::R16_SFLOAT, VK_FORMAT_R16_SFLOAT)
	ENTRY(EFormat::R16G16_UINT, VK_FORMAT_R16G16_UINT)
	ENTRY(EFormat::R16G16_SINT, VK_FORMAT_R16G16_SINT)
	ENTRY(EFormat::R16G16_SFLOAT, VK_FORMAT_R16G16_SFLOAT)
	ENTRY(EFormat::R16G16B16_UINT, VK_FORMAT_R16G16B16_UINT)
	ENTRY(EFormat::R16G16B16_SINT, VK_FORMAT_R16G16B16_SINT)
	ENTRY(EFormat::R16G16B16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT)
	ENTRY(EFormat::R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT)
	ENTRY(EFormat::R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT)
	ENTRY(EFormat::R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT)
	ENTRY(EFormat::R32_UINT, VK_FORMAT_R32_UINT)
	ENTRY(EFormat::R32_SINT, VK_FORMAT_R32_SINT)
	ENTRY(EFormat::R32_SFLOAT, VK_FORMAT_R32_SFLOAT)
	ENTRY(EFormat::R32G32_UINT, VK_FORMAT_R32G32_UINT)
	ENTRY(EFormat::R32G32_SINT, VK_FORMAT_R32G32_SINT)
	ENTRY(EFormat::R32G32_SFLOAT, VK_FORMAT_R32G32_SFLOAT)
	ENTRY(EFormat::R32G32B32_UINT, VK_FORMAT_R32G32B32_UINT)
	ENTRY(EFormat::R32G32B32_SINT, VK_FORMAT_R32G32B32_SINT)
	ENTRY(EFormat::R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT)
	ENTRY(EFormat::R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT)
	ENTRY(EFormat::R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SINT)
	ENTRY(EFormat::R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT)
	ENTRY(EFormat::D16_UNORM, VK_FORMAT_D16_UNORM)
	ENTRY(EFormat::D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT)
	ENTRY(EFormat::D32_SFLOAT, VK_FORMAT_D32_SFLOAT)
	ENTRY(EFormat::S8_UINT, VK_FORMAT_S8_UINT)
	ENTRY(EFormat::D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT)
	ENTRY(EFormat::BC2_UNORM_BLOCK, VK_FORMAT_BC2_UNORM_BLOCK)
};

VulkanImageView::VulkanImageView(VulkanDevice& Device, VkImageView ImageView, EFormat Format)
	: Device(&Device)
	, ImageView(ImageView)
	, Format(Format)
{
}

VulkanImageView::VulkanImageView(VulkanImageView&& Other)
	: Device(Other.Device)
	, ImageView(std::exchange(Other.ImageView, nullptr))
	, Format(Other.Format)
{
}

VulkanImageView& VulkanImageView::operator=(VulkanImageView&& Other)
{
	Device = std::exchange(Other.Device, nullptr);
	ImageView = std::exchange(Other.ImageView, nullptr);
	Format = Other.Format;
	return *this;
}

VulkanImageView::~VulkanImageView()
{
	if (ImageView)
	{
		vkDestroyImageView(*Device, ImageView, nullptr);
	}
}

VulkanImage::VulkanImage(VulkanDevice& Device
	, VkImage Image
	, VkDeviceMemory Memory
	, EFormat Format
	, uint32 Width
	, uint32 Height
	, uint32 Depth
	, EImageUsage UsageFlags
	, uint32 MipLevels) 
	: Device(Device)
	, Image(Image)
	, Memory(Memory)
	, drm::ImagePrivate(Format, Width, Height, Depth, UsageFlags, MipLevels)
{
	ImageView = Device.CreateImageView(*this, 0, MipLevels, 0, Any(GetUsage() & EImageUsage::Cubemap) ? 6 : 1);
}

VulkanImage::VulkanImage(VulkanImage&& Other)
	: Image(std::exchange(Other.Image, nullptr))
	, ImageView(std::move(Other.ImageView))
	, Memory(Other.Memory)
	, Device(Other.Device)
	, ImagePrivate(Other)
{
}

VulkanImage& VulkanImage::operator=(VulkanImage&& Other)
{
	Image = std::exchange(Other.Image, nullptr);
	ImageView = std::move(Other.ImageView);
	Memory = Other.Memory;
	Device = Other.Device;
	Format = Other.Format;
	Width = Other.Width;
	Height = Other.Height;
	Depth = Other.Depth;
	Usage = Other.Usage;
	MipLevels = Other.MipLevels;
	return *this;
}

VulkanImage::~VulkanImage()
{
	if (Image != nullptr)
	{
		vkDestroyImage(Device, Image, nullptr);
		vkFreeMemory(Device, Memory, nullptr);
	}
}

VulkanImage::operator VkImage() { return Image; }

VkFormat VulkanImage::GetVulkanFormat(EFormat Format)
{
	return GetValue(VulkanFormat, Format);
}

EFormat VulkanImage::GetEngineFormat(VkFormat Format)
{
	return GetKey(VulkanFormat, Format);
}

VkImageLayout VulkanImage::GetVulkanLayout(EImageLayout Layout)
{
	static const HashTable<EImageLayout, VkImageLayout> VulkanLayout =
	{
		ENTRY(EImageLayout::Undefined, VK_IMAGE_LAYOUT_UNDEFINED)
		ENTRY(EImageLayout::General, VK_IMAGE_LAYOUT_GENERAL)
		ENTRY(EImageLayout::ColorAttachmentOptimal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		ENTRY(EImageLayout::DepthWriteStencilWrite, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		ENTRY(EImageLayout::DepthReadStencilRead, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
		ENTRY(EImageLayout::ShaderReadOnlyOptimal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		ENTRY(EImageLayout::TransferSrcOptimal, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		ENTRY(EImageLayout::TransferDstOptimal, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		ENTRY(EImageLayout::Preinitialized, VK_IMAGE_LAYOUT_PREINITIALIZED)
		ENTRY(EImageLayout::DepthReadStencilWrite, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL)
		ENTRY(EImageLayout::DepthWriteStencilRead, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL)
		ENTRY(EImageLayout::Present, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	};

	return GetValue(VulkanLayout, Layout);
}

bool VulkanImage::IsDepthLayout(VkImageLayout Layout)
{
	static const std::unordered_set<VkImageLayout> VulkanDepthLayouts =
	{
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
	};

	return Contains(VulkanDepthLayouts, Layout);
}

VkFilter VulkanImage::GetVulkanFilter(EFilter Filter)
{
	static const VkFilter VulkanFilters[] =
	{
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
		VK_FILTER_CUBIC_IMG
	};

	return VulkanFilters[(uint32)Filter];
}

VkFormat VulkanImage::GetVulkanFormat() const
{
	return GetValue(VulkanFormat, GetFormat());
}

VkImageAspectFlags VulkanImage::GetVulkanAspect() const
{
	VkFlags Flags = 0;
	if (IsDepthStencil())
	{
		Flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else if (IsDepth())
	{
		Flags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (IsStencil())
	{
		Flags = VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		Flags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	return Flags;
}

static VkFormat FindSupportedFormat(VulkanDevice& Device, const std::vector<VkFormat>& Candidates, VkImageTiling Tiling, VkFormatFeatureFlags Features)
{
	for (VkFormat Format : Candidates)
	{
		VkFormatProperties Props;
		vkGetPhysicalDeviceFormatProperties(Device.GetPhysicalDevice(), Format, &Props);

		if (Tiling == VK_IMAGE_TILING_LINEAR && (Props.linearTilingFeatures & Features) == Features)
		{
			return Format;
		}
		else if (Tiling == VK_IMAGE_TILING_OPTIMAL && (Props.optimalTilingFeatures & Features) == Features)
		{
			return Format;
		}
	}

	fail("Failed to find supported format.");
}

VkFormat VulkanImage::FindSupportedDepthFormat(VulkanDevice& Device, EFormat Format)
{
	const auto Candidates = [&] () -> std::vector<VkFormat>
	{
		if (drm::Image::IsDepthStencil(Format))
		{
			return
			{
				VulkanImage::GetVulkanFormat(Format),
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT
			};
		}
		else
		{
			return
			{
				VulkanImage::GetVulkanFormat(Format),
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D16_UNORM
			};
		}
	}();

	return FindSupportedFormat(Device, Candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

const drm::Sampler* VulkanCache::GetSampler(const SamplerDesc& SamplerDesc)
{
	const Crc Crc = CalculateCrc(&SamplerDesc, sizeof(SamplerDesc));

	if (auto SamplerIter = SamplerCache.find(Crc); SamplerIter != SamplerCache.end())
	{
		return &SamplerIter->second;
	}
	else
	{
		SamplerCache.emplace(Crc, VulkanSampler(Device, SamplerDesc));
		return &SamplerCache.at(Crc);
	}
}

VulkanSampler::VulkanSampler(VulkanDevice& Device, const SamplerDesc& SamplerDesc)
{
	static const VkSamplerMipmapMode VulkanMipmapModes[] =
	{
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	};

	static const VkSamplerAddressMode VulkanAddressModes[] =
	{
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
	};

	VkFilter Filter = VulkanImage::GetVulkanFilter(SamplerDesc.Filter);
	VkSamplerMipmapMode SMM = VulkanMipmapModes[static_cast<uint32>(SamplerDesc.SMM)];
	VkSamplerAddressMode SAM = VulkanAddressModes[static_cast<uint32>(SamplerDesc.SAM)];

	VkSamplerCreateInfo SamplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	SamplerInfo.magFilter = Filter;
	SamplerInfo.minFilter = Filter;
	SamplerInfo.mipmapMode = SMM;
	SamplerInfo.addressModeU = SAM;
	SamplerInfo.addressModeV = SAM;
	SamplerInfo.addressModeW = SAM;
	SamplerInfo.anisotropyEnable = Device.GetFeatures().samplerAnisotropy;
	SamplerInfo.maxAnisotropy = Device.GetFeatures().samplerAnisotropy ? Device.GetProperties().limits.maxSamplerAnisotropy : 1.0f;
	SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	SamplerInfo.compareEnable = VK_FALSE;
	SamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	SamplerInfo.minLod = SamplerDesc.MinLod;
	SamplerInfo.maxLod = SamplerDesc.MaxLod;

	vulkan(vkCreateSampler(Device, &SamplerInfo, nullptr, &Sampler));
}

static VkImageLayout ChooseImageLayout(EFormat Format)
{
	if (drm::ImagePrivate::IsColor(Format))
	{
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	else if (drm::ImagePrivate::IsDepthStencil(Format))
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	else if (drm::ImagePrivate::IsDepth(Format))
	{
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else // Image.IsStencil()
	{
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	}
}

VulkanDescriptorImageInfo::VulkanDescriptorImageInfo(const VulkanImageView& ImageView, const VulkanSampler* Sampler)
{
	DescriptorImageInfo.sampler = Sampler ? Sampler->GetHandle() : nullptr;
	DescriptorImageInfo.imageView = ImageView.GetNativeHandle();
	DescriptorImageInfo.imageLayout = Sampler ? ChooseImageLayout(ImageView.GetFormat()) : VK_IMAGE_LAYOUT_GENERAL;
}

VulkanDescriptorImageInfo::VulkanDescriptorImageInfo(const VulkanImage& Image, const VulkanSampler* Sampler)
{
	DescriptorImageInfo.sampler = Sampler ? Sampler->GetHandle() : nullptr;
	DescriptorImageInfo.imageView = Image.ImageView.GetNativeHandle();
	DescriptorImageInfo.imageLayout = Sampler ? ChooseImageLayout(Image.ImageView.GetFormat()) : VK_IMAGE_LAYOUT_GENERAL;
}

void VulkanDescriptorImageInfo::SetImage(const VulkanImage& Image)
{
	DescriptorImageInfo.imageView = Image.ImageView.GetNativeHandle();
	DescriptorImageInfo.imageLayout = DescriptorImageInfo.sampler ? ChooseImageLayout(Image.GetFormat()) : VK_IMAGE_LAYOUT_GENERAL;
}

bool VulkanDescriptorImageInfo::operator==(const VulkanImage& Image)
{
	return DescriptorImageInfo.imageView == Image.ImageView.GetNativeHandle();
}

bool VulkanDescriptorImageInfo::operator!=(const VulkanImage& Image)
{
	return !(*this == Image);
}