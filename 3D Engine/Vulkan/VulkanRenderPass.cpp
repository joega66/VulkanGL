#include "VulkanDevice.h"
#include "VulkanDRM.h"

static CAST(drm::RenderTargetView, VulkanRenderTargetView);
static CAST(drm::Image, VulkanImage);

VulkanDevice::VulkanRenderPassCacheInfo::VulkanRenderPassCacheInfo(const RenderPassInitializer & RPInit)
{
	NumRenderTargets = RPInit.NumRenderTargets;

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < NumRenderTargets; ColorTargetIndex++)
	{
		ColorTargets[ColorTargetIndex] = ResourceCast(RPInit.ColorTargets[ColorTargetIndex]->Image)->Image;
	}

	DepthTarget = ResourceCast(RPInit.DepthTarget->Image)->Image;
	DepthStencilTransition = RPInit.DepthStencilTransition;
}

bool VulkanDevice::VulkanRenderPassCacheInfo::operator==(const VulkanRenderPassCacheInfo& Other)
{
	if (NumRenderTargets != Other.NumRenderTargets)
	{
		return false;
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < NumRenderTargets; ColorTargetIndex++)
	{
		if (ColorTargets[ColorTargetIndex] != Other.ColorTargets[ColorTargetIndex])
		{
			return false;
		}
	}

	return DepthTarget == Other.DepthTarget && DepthStencilTransition == Other.DepthStencilTransition;
}

std::pair<VkRenderPass, VkFramebuffer> VulkanDevice::GetRenderPass(const RenderPassInitializer& RPInit)
{
	VkRenderPass RenderPass = VK_NULL_HANDLE;
	VkFramebuffer Framebuffer;
	VulkanRenderPassCacheInfo CacheInfo(RPInit);

	// Find or create the render pass.
	for (const auto&[OtherCacheInfo, CachedRenderPass, CachedFramebuffer] : RenderPassCache)
	{
		if (CacheInfo == OtherCacheInfo)
		{
			RenderPass = CachedRenderPass;
			Framebuffer = CachedFramebuffer;
			break;
		}
	}

	if (RenderPass == VK_NULL_HANDLE)
	{
		std::tie(RenderPass, Framebuffer) = CreateRenderPass(RPInit);

		RenderPassCache.push_back({ CacheInfo, RenderPass, Framebuffer });
	}

	return { RenderPass, Framebuffer };
}

std::pair<VkRenderPass, VkFramebuffer> VulkanDevice::CreateRenderPass(const RenderPassInitializer& RPInit)
{
	VkRenderPass RenderPass;

	{
		check(RPInit.NumRenderTargets < MaxRenderTargets, "Trying to set too many render targets.");

		std::vector<VkAttachmentDescription> Descriptions;
		std::vector<VkAttachmentReference> ColorRefs(RPInit.NumRenderTargets);
		VkAttachmentReference DepthRef = {};

		if (RPInit.DepthTarget)
		{
			Descriptions.resize(RPInit.NumRenderTargets + 1);
		}
		else
		{
			Descriptions.resize(RPInit.NumRenderTargets);
		}

		for (uint32 i = 0; i < RPInit.NumRenderTargets; i++)
		{
			check(RPInit.ColorTargets[i], "Color target is null.");

			VulkanRenderTargetViewRef ColorTarget = ResourceCast(RPInit.ColorTargets[i]);
			VulkanImageRef Image = ResourceCast(ColorTarget->Image);

			check(Image && Any(Image->Usage & EResourceUsage::RenderTargetable), "Color target is invalid.");
			check(Image->IsColor(), "Color target was not created in color format.");

			Image->Layout = [&] ()
			{
				if (Image == drm::GetSurface())
				{
					return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				}
				else if (Any(Image->Usage & EResourceUsage::ShaderResource))
				{
					return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
				else if (Any(Image->Usage & EResourceUsage::RenderTargetable))
				{
					return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				}
				else
				{
					fail("Underspecified Resource Create Flags.");
				}
			}();

			VkAttachmentDescription ColorDescription = {};
			ColorDescription.format = Image->GetVulkanFormat();
			ColorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			ColorDescription.loadOp = ColorTarget->GetVulkanLoadOp();
			ColorDescription.storeOp = ColorTarget->GetVulkanStoreOp();
			ColorDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ColorDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ColorDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			ColorDescription.finalLayout = Image->Layout;
			Descriptions[i] = ColorDescription;

			VkAttachmentReference ColorRef = {};
			ColorRef.attachment = i;
			ColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			ColorRefs[i] = ColorRef;
		}

		if (RPInit.DepthStencilTransition != EDepthStencilTransition::None)
		{
			VulkanRenderTargetViewRef DepthTarget = ResourceCast(RPInit.DepthTarget);
			check(DepthTarget, "Depth target is invalid.");

			VulkanImageRef DepthImage = ResourceCast(DepthTarget->Image);
			check(DepthImage && Any(DepthImage->Usage & EResourceUsage::RenderTargetable), "Depth target is invalid.");
			check(DepthImage->IsDepth() || DepthImage->IsStencil(), "Depth target was not created in a depth layout.");

			VkImageLayout FinalLayout = [&] ()
			{
				if (RPInit.DepthStencilTransition == EDepthStencilTransition::DepthReadStencilRead)
				{
					check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				else if (RPInit.DepthStencilTransition == EDepthStencilTransition::DepthReadStencilWrite)
				{
					check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
					return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else if (RPInit.DepthStencilTransition == EDepthStencilTransition::DepthWriteStencilRead)
				{
					check(Any(DepthImage->Usage & EResourceUsage::ShaderResource), "Depth Image must be created with EResourceUsage::ShaderResource.");
					return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
				}
				else // DepthWriteStencilWrite
				{
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
			}();

			VkAttachmentLoadOp LoadOp = DepthTarget->GetVulkanLoadOp();
			VkAttachmentStoreOp StoreOp = DepthTarget->GetVulkanStoreOp();

			VkAttachmentDescription DepthDescription = {};
			DepthDescription.format = DepthImage->GetVulkanFormat();
			DepthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			DepthDescription.loadOp = DepthImage->IsDepth() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.storeOp = DepthImage->IsDepth() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.stencilLoadOp = DepthImage->IsStencil() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.stencilStoreOp = DepthImage->IsStencil() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.initialLayout = DepthImage->Layout;
			DepthDescription.finalLayout = FinalLayout;
			Descriptions[RPInit.NumRenderTargets] = DepthDescription;

			DepthRef.attachment = RPInit.NumRenderTargets;
			DepthRef.layout = FinalLayout;

			DepthImage->Layout = FinalLayout;
		}

		VkSubpassDescription Subpass = {};
		Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpass.pColorAttachments = ColorRefs.data();
		Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
		Subpass.pDepthStencilAttachment = !RPInit.DepthTarget ? nullptr : &DepthRef;

		std::array<VkSubpassDependency, 2> Dependencies;

		Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		Dependencies[0].dstSubpass = 0;
		Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		Dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		Dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		Dependencies[1].srcSubpass = 0;
		Dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		Dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		Dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		Dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		Dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		Dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo RenderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		RenderPassInfo.pAttachments = Descriptions.data();
		RenderPassInfo.attachmentCount = static_cast<uint32>(Descriptions.size());
		RenderPassInfo.subpassCount = 1;
		RenderPassInfo.pSubpasses = &Subpass;
		RenderPassInfo.pDependencies = Dependencies.data();
		RenderPassInfo.dependencyCount = static_cast<uint32>(Dependencies.size());

		vulkan(vkCreateRenderPass(Device, &RenderPassInfo, nullptr, &RenderPass));
	}

	const uint32 NumRTs = RPInit.NumRenderTargets;
	std::vector<VkImageView> AttachmentViews;

	if (RPInit.DepthTarget)
	{
		AttachmentViews.resize(NumRTs + 1);
	}
	else
	{
		AttachmentViews.resize(NumRTs);
	}

	for (uint32 i = 0; i < NumRTs; i++)
	{
		VulkanRenderTargetViewRef ColorTarget = ResourceCast(RPInit.ColorTargets[i]);
		VulkanImageRef Image = ResourceCast(ColorTarget->Image);

		AttachmentViews[i] = Image->ImageView;
	}

	if (RPInit.DepthTarget)
	{
		VulkanRenderTargetViewRef DepthTarget = ResourceCast(RPInit.DepthTarget);
		VulkanImageRef Image = ResourceCast(DepthTarget->Image);

		AttachmentViews[NumRTs] = Image->ImageView;
	}

	drm::ImageRef Image = RPInit.DepthTarget ? RPInit.DepthTarget->Image : RPInit.ColorTargets[0]->Image;

	VkFramebufferCreateInfo FramebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	FramebufferInfo.renderPass = RenderPass;
	FramebufferInfo.pAttachments = AttachmentViews.data();
	FramebufferInfo.attachmentCount = AttachmentViews.size();
	FramebufferInfo.width = Image->Width;
	FramebufferInfo.height = Image->Height;
	FramebufferInfo.layers = 1;

	VkFramebuffer Framebuffer;
	vulkan(vkCreateFramebuffer(Device, &FramebufferInfo, nullptr, &Framebuffer));

	return { RenderPass, Framebuffer };
}