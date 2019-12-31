#include "VulkanDevice.h"
#include "VulkanDRM.h"

VulkanDevice::VulkanRenderPassHash::MinRenderTargetView::MinRenderTargetView(const drm::RenderTargetView& RTView)
{
	const VulkanImageRef& VulkanImage = ResourceCast(RTView.Image);
	Image = VulkanImage->Image;
	LoadAction = RTView.LoadAction;
	StoreAction = RTView.StoreAction;
	InitialLayout = RTView.Image->Layout;
	FinalLayout = RTView.FinalLayout;
}

bool VulkanDevice::VulkanRenderPassHash::MinRenderTargetView::operator==(const MinRenderTargetView& Other)
{
	return Image == Other.Image &&
		LoadAction == Other.LoadAction &&
		StoreAction == Other.StoreAction &&
		InitialLayout == Other.InitialLayout &&
		FinalLayout == Other.FinalLayout;
}

VulkanDevice::VulkanRenderPassHash::VulkanRenderPassHash(const RenderPassInitializer& RPInit)
{
	if (RPInit.DepthTarget.Image)
	{
		DepthTarget = MinRenderTargetView(RPInit.DepthTarget);
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < RPInit.NumRenderTargets; ColorTargetIndex++)
	{
		ColorTargets.push_back(RPInit.ColorTargets[ColorTargetIndex]);
	}
}

bool VulkanDevice::VulkanRenderPassHash::operator==(const VulkanRenderPassHash& Other)
{
	if (ColorTargets.size() != Other.ColorTargets.size())
	{
		return false;
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < ColorTargets.size(); ColorTargetIndex++)
	{
		if (!(ColorTargets[ColorTargetIndex] == Other.ColorTargets[ColorTargetIndex]))
		{
			return false;
		}
	}

	return DepthTarget == Other.DepthTarget;
}

std::pair<VkRenderPass, VkFramebuffer> VulkanDevice::GetRenderPass(const RenderPassInitializer& RPInit)
{
	VkRenderPass RenderPass = VK_NULL_HANDLE;
	VkFramebuffer Framebuffer;
	VulkanRenderPassHash RenderPassHash(RPInit);

	// Find or create the render pass.
	for (const auto&[OtherRenderPassHash, CachedRenderPass, CachedFramebuffer] : RenderPassCache)
	{
		if (RenderPassHash == OtherRenderPassHash)
		{
			RenderPass = CachedRenderPass;
			Framebuffer = CachedFramebuffer;
			break;
		}
	}

	if (RenderPass == VK_NULL_HANDLE)
	{
		std::tie(RenderPass, Framebuffer) = CreateRenderPass(RPInit);
		RenderPassCache.push_back({ RenderPassHash, RenderPass, Framebuffer });
	}

	return { RenderPass, Framebuffer };
}

static VkAttachmentLoadOp GetVulkanLoadOp(ELoadAction LoadAction)
{
	if (LoadAction == ELoadAction::Clear)
	{
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
	else if (LoadAction == ELoadAction::Load)
	{
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	else // ELoadAction::DontCare
	{
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}

static VkAttachmentStoreOp GetVulkanStoreOp(EStoreAction StoreAction)
{
	if (StoreAction == EStoreAction::Store)
	{
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
	else // EStoreAction::DontCare
	{
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
}

std::pair<VkRenderPass, VkFramebuffer> VulkanDevice::CreateRenderPass(const RenderPassInitializer& RPInit)
{
	VkRenderPass RenderPass;

	{
		check(RPInit.NumRenderTargets < MaxRenderTargets, "Trying to set too many render targets.");

		std::vector<VkAttachmentDescription> Descriptions;
		std::vector<VkAttachmentReference> ColorRefs(RPInit.NumRenderTargets);
		VkAttachmentReference DepthRef = {};

		if (RPInit.DepthTarget.Image)
		{
			Descriptions.resize(RPInit.NumRenderTargets + 1);
		}
		else
		{
			Descriptions.resize(RPInit.NumRenderTargets);
		}

		for (uint32 i = 0; i < RPInit.NumRenderTargets; i++)
		{
			const drm::RenderTargetView& ColorTarget = RPInit.ColorTargets[i];

			VulkanImageRef Image = ResourceCast(ColorTarget.Image);

			check(Image && Any(Image->Usage & EImageUsage::Attachment), "Color target is invalid.");
			check(Image->IsColor(), "Color target was not created in color format.");

			VkAttachmentDescription ColorDescription = {};
			ColorDescription.format = Image->GetVulkanFormat();
			ColorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			ColorDescription.loadOp = GetVulkanLoadOp(ColorTarget.LoadAction);
			ColorDescription.storeOp = GetVulkanStoreOp(ColorTarget.StoreAction);
			ColorDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ColorDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ColorDescription.initialLayout = Image->GetVulkanLayout();
			ColorDescription.finalLayout = VulkanImage::GetVulkanLayout(ColorTarget.FinalLayout);
			Descriptions[i] = ColorDescription;

			ColorRefs[i].attachment = i;
			ColorRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			Image->Layout = ColorTarget.FinalLayout;
		}

		if (RPInit.DepthTarget.Image)
		{
			VulkanImageRef DepthImage = ResourceCast(RPInit.DepthTarget.Image);
			check(DepthImage && Any(DepthImage->Usage & EImageUsage::Attachment), "Depth target is invalid.");
			check(DepthImage->IsDepth() || DepthImage->IsStencil(), "Depth target was not created in a depth layout.");

			VkAttachmentLoadOp LoadOp = GetVulkanLoadOp(RPInit.DepthTarget.LoadAction);
			VkAttachmentStoreOp StoreOp = GetVulkanStoreOp(RPInit.DepthTarget.StoreAction);

			VkAttachmentDescription DepthDescription = {};
			DepthDescription.format = DepthImage->GetVulkanFormat();
			DepthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			DepthDescription.loadOp = DepthImage->IsDepth() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.storeOp = DepthImage->IsDepth() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.stencilLoadOp = DepthImage->IsStencil() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.stencilStoreOp = DepthImage->IsStencil() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.initialLayout = DepthImage->GetVulkanLayout();
			DepthDescription.finalLayout = VulkanImage::GetVulkanLayout(RPInit.DepthTarget.FinalLayout);
			Descriptions[RPInit.NumRenderTargets] = DepthDescription;

			DepthRef.attachment = RPInit.NumRenderTargets;
			DepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			DepthImage->Layout = RPInit.DepthTarget.FinalLayout;
		}

		VkSubpassDescription Subpass = {};
		Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpass.pColorAttachments = ColorRefs.data();
		Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
		Subpass.pDepthStencilAttachment = !RPInit.DepthTarget.Image ? nullptr : &DepthRef;

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

	std::vector<VkImageView> AttachmentViews;

	if (RPInit.DepthTarget.Image)
	{
		AttachmentViews.resize(RPInit.NumRenderTargets + 1);
	}
	else
	{
		AttachmentViews.resize(RPInit.NumRenderTargets);
	}

	for (uint32 ColorTargetIndex = 0; ColorTargetIndex < RPInit.NumRenderTargets; ColorTargetIndex++)
	{
		VulkanImageRef Image = ResourceCast(RPInit.ColorTargets[ColorTargetIndex].Image);
		AttachmentViews[ColorTargetIndex] = Image->ImageView;
	}

	if (RPInit.DepthTarget.Image)
	{
		VulkanImageRef Image = ResourceCast(RPInit.DepthTarget.Image);
		AttachmentViews[RPInit.NumRenderTargets] = Image->ImageView;
	}

	VkFramebuffer Framebuffer = VK_NULL_HANDLE;

	const drm::RenderTargetView& RTView = RPInit.DepthTarget.Image ? RPInit.DepthTarget : RPInit.ColorTargets[0];

	VkFramebufferCreateInfo FramebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	FramebufferInfo.renderPass = RenderPass;
	FramebufferInfo.pAttachments = AttachmentViews.data();
	FramebufferInfo.attachmentCount = AttachmentViews.size();
	FramebufferInfo.width = RPInit.RenderArea.Extent.x;
	FramebufferInfo.height = RPInit.RenderArea.Extent.y;
	FramebufferInfo.layers = 1;

	vulkan(vkCreateFramebuffer(Device, &FramebufferInfo, nullptr, &Framebuffer));

	return { RenderPass, Framebuffer };
}