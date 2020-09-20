#include "VulkanImage.h"
#include "VulkanDevice.h"
#include <unordered_set>

static std::unordered_map<EFormat, VkFormat> gEngineToVulkanFormat =
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

static std::unordered_map<VkFormat, EFormat> gVulkanToEngineFormat = [&] ()
{
	std::unordered_map<VkFormat, EFormat> ReverseMap;
	std::for_each(gEngineToVulkanFormat.begin(), gEngineToVulkanFormat.end(), [&ReverseMap] (const auto& Pair)
	{
		ReverseMap[Pair.second] = Pair.first;
	});
	return ReverseMap;
}();

VulkanImageView::VulkanImageView(VulkanDevice& device, VkImageView imageView, EFormat format)
	: _Device(device)
	, _ImageView(imageView)
	, _Format(format)
{
}

VulkanImageView::VulkanImageView(VulkanImageView&& other)
	: _Device(other._Device)
	, _ImageView(std::exchange(other._ImageView, nullptr))
	, _Format(other._Format)
{
}

VulkanImageView& VulkanImageView::operator=(VulkanImageView&& other)
{
	_Device = other._Device;
	_ImageView = std::exchange(other._ImageView, nullptr);
	_Format = other._Format;
	return *this;
}

VulkanImageView::~VulkanImageView()
{
	if (_ImageView)
	{
		vkDestroyImageView(_Device, _ImageView, nullptr);
	}
}

VulkanImage::VulkanImage(VulkanDevice& Device
	, VmaAllocator allocator
	, VmaAllocation allocation
	, const VmaAllocationInfo& allocationInfo
	, VkImage image
	, EFormat format
	, uint32 width
	, uint32 height
	, uint32 depth
	, EImageUsage usage
	, uint32 mipLevels) 
	: _Allocator(allocator)
	, _Allocation(allocation)
	, _AllocationInfo(allocationInfo)
	, _Image(image)
	, gpu::ImagePrivate(format, width, height, depth, usage, mipLevels)
{
	_ImageView = Device.CreateImageView(*this, 0, mipLevels, 0, Any(usage & EImageUsage::Cubemap) ? 6 : 1);

	if (Any(usage & EImageUsage::Sampled))
	{
		_TextureID = Device.CreateTextureID(_ImageView);
	}
	if (Any(usage & EImageUsage::Storage))
	{
		_ImageID = Device.CreateImageID(_ImageView);
	}
}

VulkanImage::VulkanImage(VulkanImage&& other)
	: _Allocator(std::exchange(other._Allocator, nullptr))
	, _Allocation(std::exchange(other._Allocation, nullptr))
	, _AllocationInfo(std::exchange(other._AllocationInfo, {}))
	, _Image(std::exchange(other._Image, nullptr))
	, _ImageView(std::move(other._ImageView))
	, _TextureID(std::move(other._TextureID))
	, _ImageID(std::move(other._ImageID))
	, ImagePrivate(other)
{
}

VulkanImage& VulkanImage::operator=(VulkanImage&& other)
{
	_Allocator = std::exchange(other._Allocator, nullptr);
	_Allocation = std::exchange(other._Allocation, nullptr);
	_AllocationInfo = std::exchange(other._AllocationInfo, {});
	_Image = std::exchange(other._Image, nullptr);
	_ImageView = std::move(other._ImageView);
	_TextureID = std::move(other._TextureID);
	_ImageID = std::move(other._ImageID);
	_Format = other._Format;
	_Width = other._Width;
	_Height = other._Height;
	_Depth = other._Depth;
	_Usage = other._Usage;
	_MipLevels = other._MipLevels;
	return *this;
}

VulkanImage::~VulkanImage()
{
	if (_Allocation != nullptr)
	{
		_TextureID.Release();
		_ImageID.Release();
		vmaDestroyImage(_Allocator, _Image, _Allocation);
	}
}

VkFormat VulkanImage::GetVulkanFormat(EFormat format)
{
	return gEngineToVulkanFormat[format];
}

EFormat VulkanImage::GetEngineFormat(VkFormat format)
{
	return gVulkanToEngineFormat[format];
}

VkImageLayout VulkanImage::GetLayout(EImageLayout layout)
{
	static std::unordered_map<EImageLayout, VkImageLayout> vulkanLayout =
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

	return vulkanLayout[layout];
}

bool VulkanImage::IsDepthLayout(VkImageLayout layout)
{
	static const std::unordered_set<VkImageLayout> vulkanDepthLayouts =
	{
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
		, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
	};
	return vulkanDepthLayouts.contains(layout);
}

VkFilter VulkanImage::GetVulkanFilter(EFilter filter)
{
	static const VkFilter vulkanFilters[] =
	{
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
		VK_FILTER_CUBIC_IMG
	};

	return vulkanFilters[static_cast<uint32>(filter)];
}

VkFormat VulkanImage::GetVulkanFormat() const
{
	return gEngineToVulkanFormat[_Format];
}

VkImageAspectFlags VulkanImage::GetVulkanAspect() const
{
	VkImageAspectFlags flags = 0;
	if (IsDepthStencil())
	{
		flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else if (IsDepth())
	{
		flags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (IsStencil())
	{
		flags = VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		flags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	return flags;
}

static VkFormat FindSupportedFormat(VulkanDevice& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	fail("Failed to find supported format.");
}

VkFormat VulkanImage::FindSupportedDepthFormat(VulkanDevice& device, EFormat format)
{
	const auto candidates = [&] () -> std::vector<VkFormat>
	{
		if (gpu::Image::IsDepthStencil(format))
		{
			return
			{
				VulkanImage::GetVulkanFormat(format),
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT
			};
		}
		else
		{
			return
			{
				VulkanImage::GetVulkanFormat(format),
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D16_UNORM
			};
		}
	}();

	return FindSupportedFormat(device, candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VulkanSampler::VulkanSampler(VulkanDevice& device, const SamplerDesc& samplerDesc)
{
	static const VkSamplerMipmapMode vulkanMipmapModes[] =
	{
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	};

	static const VkSamplerAddressMode vulkanAddressModes[] =
	{
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
	};

	const VkFilter filter = VulkanImage::GetVulkanFilter(samplerDesc.filter);
	const VkSamplerMipmapMode smm = vulkanMipmapModes[static_cast<uint32>(samplerDesc.smm)];
	const VkSamplerAddressMode sam = vulkanAddressModes[static_cast<uint32>(samplerDesc.sam)];

	VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.mipmapMode = smm;
	samplerInfo.addressModeU = sam;
	samplerInfo.addressModeV = sam;
	samplerInfo.addressModeW = sam;
	samplerInfo.anisotropyEnable = device.GetFeatures().samplerAnisotropy;
	samplerInfo.maxAnisotropy = device.GetFeatures().samplerAnisotropy ? device.GetProperties().limits.maxSamplerAnisotropy : 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = samplerDesc.minLod;
	samplerInfo.maxLod = samplerDesc.maxLod;

	vulkan(vkCreateSampler(device, &samplerInfo, nullptr, &_Sampler));

	_SamplerID = device.GetSamplers().CreateSamplerID(*this);
}

static VkImageLayout ChooseImageLayout(EFormat format)
{
	if (gpu::ImagePrivate::IsColor(format))
	{
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	else if (gpu::ImagePrivate::IsDepthStencil(format))
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	else if (gpu::ImagePrivate::IsDepth(format))
	{
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else // Image.IsStencil()
	{
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	}
}

namespace gpu
{
	SampledImage::SampledImage(const VulkanImageView& imageView, const VulkanSampler& sampler)
	{
		_DescriptorImageInfo.sampler = sampler.GetHandle();
		_DescriptorImageInfo.imageView = imageView.GetHandle();
		_DescriptorImageInfo.imageLayout = ChooseImageLayout(imageView.GetFormat());
	}

	StorageImage::StorageImage(const VulkanImage& image)
	{
		_DescriptorImageInfo.sampler = nullptr;
		_DescriptorImageInfo.imageView = image.GetImageView().GetHandle();
		_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	StorageImage::StorageImage(const VulkanImageView& imageView)
	{
		_DescriptorImageInfo.sampler = nullptr;
		_DescriptorImageInfo.imageView = imageView.GetHandle();
		_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}
};