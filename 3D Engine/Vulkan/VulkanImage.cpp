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

VulkanImage::VulkanImage(VulkanDevice& Device
	, VkImage Image
	, VkDeviceMemory Memory
	, EFormat Format
	, uint32 Width
	, uint32 Height
	, uint32 Depth
	, EImageUsage UsageFlags)
	: Device(&Device)
	, Image(Image)
	, Memory(Memory)
	, drm::ImagePrivate(Format, Width, Height, Depth, UsageFlags)
{
	VkImageViewCreateInfo ViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	ViewInfo.image = Image;
	ViewInfo.viewType = Any(GetUsage() & EImageUsage::Cubemap) ? VK_IMAGE_VIEW_TYPE_CUBE : (Depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);
	ViewInfo.format = GetVulkanFormat();
	ViewInfo.subresourceRange.aspectMask = GetVulkanAspect();
	ViewInfo.subresourceRange.baseMipLevel = 0;
	ViewInfo.subresourceRange.levelCount = 1;
	ViewInfo.subresourceRange.baseArrayLayer = 0;
	ViewInfo.subresourceRange.layerCount = Any(GetUsage() & EImageUsage::Cubemap) ? 6 : 1;
	ViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	vulkan(vkCreateImageView(Device, &ViewInfo, nullptr, &ImageView));
}

VulkanImage::VulkanImage(VulkanImage&& Other)
	: Image(std::exchange(Other.Image, nullptr))
	, ImageView(std::exchange(Other.ImageView, nullptr))
	, Memory(std::exchange(Other.Memory, nullptr))
	, Device(std::exchange(Other.Device, nullptr))
	, ImagePrivate(Other)
{
}

VulkanImage& VulkanImage::operator=(VulkanImage&& Other)
{
	Image = (std::exchange(Other.Image, nullptr));
	ImageView = (std::exchange(Other.ImageView, nullptr));
	Memory = (std::exchange(Other.Memory, nullptr));
	Device = (std::exchange(Other.Device, nullptr));
	Format = Other.Format;
	Width = Other.Width;
	Height = Other.Height;
	Depth = Other.Depth;
	Usage = Other.Usage;
	return *this;
}

VulkanImage::~VulkanImage()
{
	if (Image != nullptr)
	{
		Device->GetCache().FreeImage(*this);
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
	return GetValue(VulkanLayout, Layout);
}

bool VulkanImage::IsDepthLayout(VkImageLayout Layout)
{
	std::unordered_set<VkImageLayout> VulkanDepthLayouts =
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

const drm::Sampler* VulkanCache::GetSampler(const SamplerDesc& SamplerDesc)
{
	if (auto SamplerIter = SamplerCache.find(SamplerDesc); SamplerIter != SamplerCache.end())
	{
		return &SamplerIter->second;
	}
	else
	{
		SamplerCache.emplace(SamplerDesc, VulkanSampler(VulkanImage::CreateSampler(Device, SamplerDesc)));
		return &SamplerCache.at(SamplerDesc);
	}
}

VkSampler VulkanImage::CreateSampler(VulkanDevice& Device, const SamplerDesc& SamplerDesc)
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

	VkFilter Filter = GetVulkanFilter(SamplerDesc.Filter);
	VkSamplerMipmapMode SMM = VulkanMipmapModes[(uint32)SamplerDesc.SMM];
	VkSamplerAddressMode SAM = VulkanAddressModes[(uint32)SamplerDesc.SAM];

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

	VkSampler Sampler;
	vulkan(vkCreateSampler(Device, &SamplerInfo, nullptr, &Sampler));

	return Sampler;
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
	const auto Candidates = [&]() -> std::vector<VkFormat>
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

void VulkanCache::FreeImage(VulkanImage& Image)
{
	vkDestroyImage(Device, Image.Image, nullptr);
	vkDestroyImageView(Device, Image.ImageView, nullptr);
	vkFreeMemory(Device, Image.Memory, nullptr);
}

VulkanSampler::VulkanSampler(VkSampler Sampler)
	: Sampler(Sampler)
{
}

VulkanImageView::VulkanImageView(const VulkanImage& Image, const VulkanSampler* Sampler)
{
	DescriptorImageInfo.sampler = Sampler ? Sampler->GetHandle() : nullptr;
	DescriptorImageInfo.imageView = Image.ImageView;
	DescriptorImageInfo.imageLayout = Sampler ? 
		(Image.IsDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
		: VK_IMAGE_LAYOUT_GENERAL;
}

void VulkanImageView::SetImage(const VulkanImage& Image)
{
	DescriptorImageInfo.imageView = Image.ImageView;
	DescriptorImageInfo.imageLayout = DescriptorImageInfo.sampler != nullptr ?
		(Image.IsDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		: VK_IMAGE_LAYOUT_GENERAL;
}

bool VulkanImageView::operator==(const VulkanImage& Image)
{
	return DescriptorImageInfo.imageView == Image.ImageView;
}

bool VulkanImageView::operator!=(const VulkanImage& Image)
{
	return !(*this == Image);
}