#include "VulkanDevice.h"
#include "VulkanDRM.h"
#include "VulkanRenderPass.h"

VulkanDevice::VulkanRenderPassHash::MinAttachmentView::MinAttachmentView(const drm::AttachmentView& AView)
{
	const VulkanImageRef& VulkanImage = ResourceCast(AView.Image);
	Image = VulkanImage->Image;
	LoadAction = AView.LoadAction;
	StoreAction = AView.StoreAction;
	InitialLayout = AView.Image->Layout;
	FinalLayout = AView.FinalLayout;
}

bool VulkanDevice::VulkanRenderPassHash::MinAttachmentView::operator==(const MinAttachmentView& Other)
{
	return Image == Other.Image &&
		LoadAction == Other.LoadAction &&
		StoreAction == Other.StoreAction &&
		InitialLayout == Other.InitialLayout &&
		FinalLayout == Other.FinalLayout;
}

VulkanDevice::VulkanRenderPassHash::VulkanRenderPassHash(const RenderPassInitializer& RPInit)
{
	if (RPInit.DepthAttachment.Image)
	{
		DepthAttachment = MinAttachmentView(RPInit.DepthAttachment);
	}

	for (uint32 ColorAttachmentIndex = 0; ColorAttachmentIndex < RPInit.NumAttachments; ColorAttachmentIndex++)
	{
		ColorAttachments.push_back(RPInit.ColorAttachments[ColorAttachmentIndex]);
	}
}

bool VulkanDevice::VulkanRenderPassHash::operator==(const VulkanRenderPassHash& Other)
{
	if (ColorAttachments.size() != Other.ColorAttachments.size())
	{
		return false;
	}

	for (uint32 ColorAttachmentIndex = 0; ColorAttachmentIndex < ColorAttachments.size(); ColorAttachmentIndex++)
	{
		if (!(ColorAttachments[ColorAttachmentIndex] == Other.ColorAttachments[ColorAttachmentIndex]))
		{
			return false;
		}
	}

	return DepthAttachment == Other.DepthAttachment;
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
		check(RPInit.NumAttachments < MaxAttachments, "Trying to set too many render targets.");

		std::vector<VkAttachmentDescription> Descriptions;
		std::vector<VkAttachmentReference> ColorRefs(RPInit.NumAttachments);
		VkAttachmentReference DepthRef = {};

		if (RPInit.DepthAttachment.Image)
		{
			Descriptions.resize(RPInit.NumAttachments + 1);
		}
		else
		{
			Descriptions.resize(RPInit.NumAttachments);
		}

		for (uint32 i = 0; i < RPInit.NumAttachments; i++)
		{
			const drm::AttachmentView& ColorAttachment = RPInit.ColorAttachments[i];

			VulkanImageRef Image = ResourceCast(ColorAttachment.Image);

			check(Image && Any(Image->Usage & EImageUsage::Attachment), "Color target is invalid.");
			check(Image->IsColor(), "Color target was not created in color format.");

			VkAttachmentDescription ColorDescription = {};
			ColorDescription.format = Image->GetVulkanFormat();
			ColorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			ColorDescription.loadOp = GetVulkanLoadOp(ColorAttachment.LoadAction);
			ColorDescription.storeOp = GetVulkanStoreOp(ColorAttachment.StoreAction);
			ColorDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ColorDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ColorDescription.initialLayout = Image->GetVulkanLayout();
			ColorDescription.finalLayout = VulkanImage::GetVulkanLayout(ColorAttachment.FinalLayout);
			Descriptions[i] = ColorDescription;

			ColorRefs[i].attachment = i;
			ColorRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		if (RPInit.DepthAttachment.Image)
		{
			VulkanImageRef DepthImage = ResourceCast(RPInit.DepthAttachment.Image);
			check(DepthImage && Any(DepthImage->Usage & EImageUsage::Attachment), "Depth target is invalid.");
			check(DepthImage->IsDepth() || DepthImage->IsStencil(), "Depth target was not created in a depth layout.");

			VkAttachmentLoadOp LoadOp = GetVulkanLoadOp(RPInit.DepthAttachment.LoadAction);
			VkAttachmentStoreOp StoreOp = GetVulkanStoreOp(RPInit.DepthAttachment.StoreAction);

			VkAttachmentDescription DepthDescription = {};
			DepthDescription.format = DepthImage->GetVulkanFormat();
			DepthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			DepthDescription.loadOp = DepthImage->IsDepth() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.storeOp = DepthImage->IsDepth() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.stencilLoadOp = DepthImage->IsStencil() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthDescription.stencilStoreOp = DepthImage->IsStencil() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthDescription.initialLayout = DepthImage->GetVulkanLayout();
			DepthDescription.finalLayout = VulkanImage::GetVulkanLayout(RPInit.DepthAttachment.FinalLayout);
			Descriptions[RPInit.NumAttachments] = DepthDescription;

			DepthRef.attachment = RPInit.NumAttachments;
			DepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		VkSubpassDescription Subpass = {};
		Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpass.pColorAttachments = ColorRefs.data();
		Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
		Subpass.pDepthStencilAttachment = !RPInit.DepthAttachment.Image ? nullptr : &DepthRef;

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

	if (RPInit.DepthAttachment.Image)
	{
		AttachmentViews.resize(RPInit.NumAttachments + 1);
	}
	else
	{
		AttachmentViews.resize(RPInit.NumAttachments);
	}

	for (uint32 ColorAttachmentIndex = 0; ColorAttachmentIndex < RPInit.NumAttachments; ColorAttachmentIndex++)
	{
		VulkanImageRef Image = ResourceCast(RPInit.ColorAttachments[ColorAttachmentIndex].Image);
		AttachmentViews[ColorAttachmentIndex] = Image->ImageView;
	}

	if (RPInit.DepthAttachment.Image)
	{
		VulkanImageRef Image = ResourceCast(RPInit.DepthAttachment.Image);
		AttachmentViews[RPInit.NumAttachments] = Image->ImageView;
	}

	VkFramebuffer Framebuffer = VK_NULL_HANDLE;

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

VulkanRenderPass::VulkanRenderPass(
	VkRenderPass RenderPass, 
	VkFramebuffer Framebuffer, 
	const VkRect2D& RenderArea,
	const std::vector<Transition>& Transitions,
	const std::vector<VkClearValue>& ClearValues,
	uint32 NumAttachments) 
	: RenderPass(RenderPass)
	, Framebuffer(Framebuffer)
	, RenderArea(RenderArea)
	, Transitions(std::move(Transitions))
	, ClearValues(std::move(ClearValues))
	, NumAttachments(NumAttachments)
{
}

void VulkanRenderPass::TransitionImages()
{
	std::for_each(Transitions.begin(), Transitions.end(), [] (Transition& Transition)
	{
		Transition.Image->Layout = Transition.Layout;
	});
}