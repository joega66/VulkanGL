#include "VulkanImage.h"
#include "VulkanDRM.h"
#include "VulkanCommands.h"
#include "RenderCommandList.h"

const HashTable<EImageFormat, VkFormat> VulkanFormat =
{
	ENTRY(EImageFormat::UNDEFINED, VK_FORMAT_UNDEFINED)
	ENTRY(EImageFormat::R8G8_UINT, VK_FORMAT_R8_UINT)
	ENTRY(EImageFormat::R8G8_SINT, VK_FORMAT_R8_SINT)
	ENTRY(EImageFormat::R8_SRGB, VK_FORMAT_R8_SRGB)
	ENTRY(EImageFormat::R8G8_UINT, VK_FORMAT_R8G8_UINT)
	ENTRY(EImageFormat::R8G8_SINT, VK_FORMAT_R8G8_SINT)
	ENTRY(EImageFormat::R8G8_SRGB, VK_FORMAT_R8G8_SRGB)
	ENTRY(EImageFormat::R8G8B8_UINT, VK_FORMAT_R8G8B8_UINT)
	ENTRY(EImageFormat::R8G8B8_SINT, VK_FORMAT_R8G8B8_SINT)
	ENTRY(EImageFormat::R8G8B8_SRGB, VK_FORMAT_R8G8B8_SRGB)
	ENTRY(EImageFormat::B8G8R8_UINT, VK_FORMAT_B8G8R8_UINT)
	ENTRY(EImageFormat::B8G8R8_SINT, VK_FORMAT_B8G8R8_SINT)
	ENTRY(EImageFormat::B8G8R8_SRGB, VK_FORMAT_B8G8R8_SRGB)
	ENTRY(EImageFormat::R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT)
	ENTRY(EImageFormat::R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT)
	ENTRY(EImageFormat::R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB)
	ENTRY(EImageFormat::R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM)
	ENTRY(EImageFormat::B8G8R8A8_UINT, VK_FORMAT_B8G8R8A8_UINT)
	ENTRY(EImageFormat::B8G8R8A8_SINT, VK_FORMAT_B8G8R8A8_SINT)
	ENTRY(EImageFormat::B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB)
	ENTRY(EImageFormat::B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)
	ENTRY(EImageFormat::R16_UINT, VK_FORMAT_R16_UINT)
	ENTRY(EImageFormat::R16_SINT, VK_FORMAT_R16_SINT)
	ENTRY(EImageFormat::R16_SFLOAT, VK_FORMAT_R16_SFLOAT)
	ENTRY(EImageFormat::R16G16_UINT, VK_FORMAT_R16G16_UINT)
	ENTRY(EImageFormat::R16G16_SINT, VK_FORMAT_R16G16_SINT)
	ENTRY(EImageFormat::R16G16_SFLOAT, VK_FORMAT_R16G16_SFLOAT)
	ENTRY(EImageFormat::R16G16B16_UINT, VK_FORMAT_R16G16B16_UINT)
	ENTRY(EImageFormat::R16G16B16_SINT, VK_FORMAT_R16G16B16_SINT)
	ENTRY(EImageFormat::R16G16B16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT)
	ENTRY(EImageFormat::R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT)
	ENTRY(EImageFormat::R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT)
	ENTRY(EImageFormat::R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT)
	ENTRY(EImageFormat::R32_UINT, VK_FORMAT_R32_UINT)
	ENTRY(EImageFormat::R32_SINT, VK_FORMAT_R32_SINT)
	ENTRY(EImageFormat::R32_SFLOAT, VK_FORMAT_R32_SFLOAT)
	ENTRY(EImageFormat::R32G32_UINT, VK_FORMAT_R32G32_UINT)
	ENTRY(EImageFormat::R32G32_SINT, VK_FORMAT_R32G32_SINT)
	ENTRY(EImageFormat::R32G32_SFLOAT, VK_FORMAT_R32G32_SFLOAT)
	ENTRY(EImageFormat::R32G32B32_UINT, VK_FORMAT_R32G32B32_UINT)
	ENTRY(EImageFormat::R32G32B32_SINT, VK_FORMAT_R32G32B32_SINT)
	ENTRY(EImageFormat::R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT)
	ENTRY(EImageFormat::R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT)
	ENTRY(EImageFormat::R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SINT)
	ENTRY(EImageFormat::R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT)
	ENTRY(EImageFormat::D16_UNORM, VK_FORMAT_D16_UNORM)
	ENTRY(EImageFormat::D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT)
	ENTRY(EImageFormat::D32_SFLOAT, VK_FORMAT_D32_SFLOAT)
	ENTRY(EImageFormat::S8_UINT, VK_FORMAT_S8_UINT)
	ENTRY(EImageFormat::D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT)
	ENTRY(EImageFormat::BC2_UNORM_BLOCK, VK_FORMAT_BC2_UNORM_BLOCK)
};

const HashTable<EImageLayout, VkImageLayout> VulkanLayout =
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

VulkanImage::VulkanImage(VulkanDevice& Device, VkImage Image, VkDeviceMemory Memory, EImageFormat Format, EImageLayout Layout, uint32 Width, uint32 Height, EResourceUsage UsageFlags, VkPipelineStageFlags Stage)
	: Device(Device)
	, Image(Image)
	, Memory(Memory)
	, Stage(Stage)
	, drm::Image(Format, Layout, Width, Height, UsageFlags)
{
	VkImageViewCreateInfo ViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	ViewInfo.image = Image;
	ViewInfo.viewType = Any(Usage & EResourceUsage::Cubemap) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	ViewInfo.format = GetVulkanFormat();
	ViewInfo.subresourceRange.aspectMask = GetVulkanAspect();
	ViewInfo.subresourceRange.baseMipLevel = 0;
	ViewInfo.subresourceRange.levelCount = 1;
	ViewInfo.subresourceRange.baseArrayLayer = 0;
	ViewInfo.subresourceRange.layerCount = Any(Usage & EResourceUsage::Cubemap) ? 6 : 1;
	ViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	vulkan(vkCreateImageView(Device, &ViewInfo, nullptr, &ImageView));
}

VulkanImage::~VulkanImage()
{
	Device.FreeImage(*this);
}

VulkanImage::operator VkImage() { return Image; }

void VulkanDRM::CreateImage(VkImage& Image, VkDeviceMemory& Memory, EImageLayout& Layout,
	uint32 Width, uint32 Height, EImageFormat& Format, EResourceUsage UsageFlags, bool bTransferDstBit)
{
	Layout = EImageLayout::Undefined;

	if (drm::Image::IsDepth(Format))
	{
		Format = VulkanImage::GetEngineFormat(VulkanImage::FindSupportedDepthFormat(Device, Format));
	}

	VkImageCreateInfo Info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	Info.imageType = VK_IMAGE_TYPE_2D;
	Info.extent.width = Width;
	Info.extent.height = Height;
	Info.extent.depth = 1;
	Info.mipLevels = 1;
	Info.arrayLayers = Any(UsageFlags & EResourceUsage::Cubemap) ? 6 : 1;
	Info.format = VulkanImage::GetVulkanFormat(Format);
	Info.tiling = VK_IMAGE_TILING_OPTIMAL;
	Info.initialLayout = VulkanImage::GetVulkanLayout(Layout);
	Info.usage = [&]()
	{
		VkFlags Usage = 0;

		if (Any(UsageFlags & EResourceUsage::RenderTargetable))
		{
			Usage |= drm::Image::IsDepth(Format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		Usage |= bTransferDstBit ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		Usage |= Any(UsageFlags & EResourceUsage::ShaderResource) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
		Usage |= Any(UsageFlags & EResourceUsage::UnorderedAccess) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

		return Usage;
	}();
	Info.flags = Any(UsageFlags & EResourceUsage::Cubemap) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	Info.samples = VK_SAMPLE_COUNT_1_BIT;
	Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vulkan(vkCreateImage(Device, &Info, nullptr, &Image));

	VkMemoryRequirements MemRequirements = {};
	vkGetImageMemoryRequirements(Device, Image, &MemRequirements);

	VkMemoryAllocateInfo MemInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	MemInfo.allocationSize = MemRequirements.size;
	MemInfo.memoryTypeIndex = Allocator.FindMemoryType(MemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vulkan(vkAllocateMemory(Device, &MemInfo, nullptr, &Memory));
	vulkan(vkBindImageMemory(Device, Image, Memory, 0));
}

VkFormat VulkanImage::GetVulkanFormat(EImageFormat Format)
{
	return GetValue(VulkanFormat, Format);
}

EImageFormat VulkanImage::GetEngineFormat(VkFormat Format)
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

VkSampler VulkanDevice::GetSampler(const SamplerState& SamplerState)
{
	if (auto SamplerIter = SamplerCache.find(SamplerState); SamplerIter != SamplerCache.end())
	{
		return SamplerIter->second;
	}
	else
	{
		VkSampler Sampler = VulkanImage::CreateSampler(*this, SamplerState);
		SamplerCache.emplace(SamplerState, Sampler);
		return Sampler;
	}
}

VkSampler VulkanImage::CreateSampler(VulkanDevice& Device, const SamplerState& SamplerState)
{
	static const VkFilter VulkanFilters[] =
	{
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
		VK_FILTER_CUBIC_IMG
	};

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

	VkFilter Filter = VulkanFilters[(uint32)SamplerState.Filter];
	VkSamplerMipmapMode SMM = VulkanMipmapModes[(uint32)SamplerState.SMM];
	VkSamplerAddressMode SAM = VulkanAddressModes[(uint32)SamplerState.SAM];

	VkSamplerCreateInfo SamplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	SamplerInfo.magFilter = Filter;
	SamplerInfo.minFilter = Filter;
	SamplerInfo.mipmapMode = SMM;
	SamplerInfo.addressModeU = SAM;
	SamplerInfo.addressModeV = SAM;
	SamplerInfo.addressModeW = SAM;
	SamplerInfo.anisotropyEnable = Device.Features.samplerAnisotropy;
	SamplerInfo.maxAnisotropy = Device.Features.samplerAnisotropy ? Device.Properties.limits.maxSamplerAnisotropy : 1.0f;
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
	return GetValue(VulkanFormat, Format);
}

VkImageLayout VulkanImage::GetVulkanLayout() const
{
	return GetValue(VulkanLayout, Layout);
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
		vkGetPhysicalDeviceFormatProperties(Device.PhysicalDevice, Format, &Props);

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

VkFormat VulkanImage::FindSupportedDepthFormat(VulkanDevice& Device, EImageFormat Format)
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

void VulkanDevice::FreeImage(VulkanImage& Image)
{
	// The image is no longer being referenced; destroy vulkan objects.
	if (Any(Image.Usage & EResourceUsage::RenderTargetable))
	{
		for (auto Iter = RenderPassCache.begin(); Iter != RenderPassCache.end();)
		{
			const auto&[CacheInfo, CachedRenderPass, CachedFramebuffer] = *Iter;

			if (Image.Image == CacheInfo.DepthTarget.Image ||
				(Image.IsColor() && std::any_of(
					CacheInfo.ColorTargets.begin(),
					CacheInfo.ColorTargets.end(),
					[&](auto ColorTarget) { return Image.Image == ColorTarget.Image; })))
			{
				vkDestroyFramebuffer(Device, CachedFramebuffer, nullptr);
				vkDestroyRenderPass(Device, CachedRenderPass, nullptr);
				Iter = RenderPassCache.erase(Iter);
			}
			else
			{
				Iter++;
			}
		}
	}

	vkDestroyImage(Device, Image.Image, nullptr);
	vkDestroyImageView(Device, Image.ImageView, nullptr);
	vkFreeMemory(Device, Image.Memory, nullptr);
}