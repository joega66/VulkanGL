#include "VulkanRenderPass.h"
#include "VulkanDevice.h"

std::pair<VkRenderPass, VkFramebuffer> VulkanCache::GetRenderPass(const RenderPassDesc& RPDesc)
{
	VkRenderPass RenderPass = VK_NULL_HANDLE;

	for (const auto&[CachedRPDesc, CachedRenderPass] : RenderPassCache)
	{
		if (RPDesc == CachedRPDesc)
		{
			RenderPass = CachedRenderPass;
			break;
		}
	}

	if (RenderPass == VK_NULL_HANDLE)
	{
		RenderPass = CreateRenderPass(RPDesc);

		RenderPassDesc CachedRPDesc = RPDesc;
		std::for_each(CachedRPDesc.ColorAttachments.begin(), CachedRPDesc.ColorAttachments.end(), [] (auto& View) { View.Image = nullptr; });
		CachedRPDesc.DepthAttachment.Image = nullptr;

		RenderPassCache.push_back({ CachedRPDesc, RenderPass });
	}

	VkFramebuffer Framebuffer = CreateFramebuffer(RenderPass, RPDesc);

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

VkRenderPass VulkanCache::CreateRenderPass(const RenderPassDesc& RPDesc)
{
	check(RPDesc.NumAttachments < RenderPassDesc::MaxAttachments, "Trying to set too many render targets.");

	std::vector<VkAttachmentDescription> Descriptions;

	std::vector<VkAttachmentReference> ColorRefs;
	ColorRefs.reserve(RPDesc.NumAttachments);

	VkAttachmentReference DepthRef = {};

	if (RPDesc.DepthAttachment.Image)
	{
		Descriptions.reserve(RPDesc.NumAttachments + 1);
	}
	else
	{
		Descriptions.reserve(RPDesc.NumAttachments);
	}

	for (uint32 AttachmentIndex = 0; AttachmentIndex < RPDesc.NumAttachments; AttachmentIndex++)
	{
		const drm::AttachmentView& ColorAttachment = RPDesc.ColorAttachments[AttachmentIndex];

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
		ColorDescription.initialLayout = VulkanImage::GetVulkanLayout(ColorAttachment.InitialLayout);
		ColorDescription.finalLayout = VulkanImage::GetVulkanLayout(ColorAttachment.FinalLayout);
		Descriptions.push_back(ColorDescription);

		ColorRefs.push_back({ AttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	}

	if (RPDesc.DepthAttachment.Image)
	{
		VulkanImageRef DepthImage = ResourceCast(RPDesc.DepthAttachment.Image);
		check(DepthImage && Any(DepthImage->Usage & EImageUsage::Attachment), "Depth target is invalid.");
		check(DepthImage->IsDepth() || DepthImage->IsStencil(), "Depth target was not created in a depth layout.");

		VkAttachmentLoadOp LoadOp = GetVulkanLoadOp(RPDesc.DepthAttachment.LoadAction);
		VkAttachmentStoreOp StoreOp = GetVulkanStoreOp(RPDesc.DepthAttachment.StoreAction);

		VkAttachmentDescription DepthDescription = {};
		DepthDescription.format = DepthImage->GetVulkanFormat();
		DepthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		DepthDescription.loadOp = DepthImage->IsDepth() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthDescription.storeOp = DepthImage->IsDepth() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthDescription.stencilLoadOp = DepthImage->IsStencil() ? LoadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthDescription.stencilStoreOp = DepthImage->IsStencil() ? StoreOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthDescription.initialLayout = VulkanImage::GetVulkanLayout(RPDesc.DepthAttachment.InitialLayout);
		DepthDescription.finalLayout = VulkanImage::GetVulkanLayout(RPDesc.DepthAttachment.FinalLayout);
		Descriptions.push_back(DepthDescription);

		DepthRef.attachment = RPDesc.NumAttachments;
		DepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.pColorAttachments = ColorRefs.data();
	Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
	Subpass.pDepthStencilAttachment = !RPDesc.DepthAttachment.Image ? nullptr : &DepthRef;

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

	VkRenderPass RenderPass;
	vulkan(vkCreateRenderPass(Device, &RenderPassInfo, nullptr, &RenderPass));

	return RenderPass;
}

VkFramebuffer VulkanCache::CreateFramebuffer(VkRenderPass RenderPass, const RenderPassDesc& RPDesc) const
{
	std::vector<VkImageView> AttachmentViews;

	if (RPDesc.DepthAttachment.Image)
	{
		AttachmentViews.reserve(RPDesc.NumAttachments + 1);
	}
	else
	{
		AttachmentViews.reserve(RPDesc.NumAttachments);
	}

	for (uint32 ColorAttachmentIndex = 0; ColorAttachmentIndex < RPDesc.NumAttachments; ColorAttachmentIndex++)
	{
		VulkanImageRef Image = ResourceCast(RPDesc.ColorAttachments[ColorAttachmentIndex].Image);
		AttachmentViews.push_back(Image->ImageView);
	}

	if (RPDesc.DepthAttachment.Image)
	{
		VulkanImageRef Image = ResourceCast(RPDesc.DepthAttachment.Image);
		AttachmentViews.push_back(Image->ImageView);
	}

	VkFramebufferCreateInfo FramebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	FramebufferInfo.renderPass = RenderPass;
	FramebufferInfo.pAttachments = AttachmentViews.data();
	FramebufferInfo.attachmentCount = AttachmentViews.size();
	FramebufferInfo.width = RPDesc.RenderArea.Extent.x;
	FramebufferInfo.height = RPDesc.RenderArea.Extent.y;
	FramebufferInfo.layers = 1;

	VkFramebuffer Framebuffer;
	vulkan(vkCreateFramebuffer(Device, &FramebufferInfo, nullptr, &Framebuffer));

	return Framebuffer;
}

VulkanRenderPass::VulkanRenderPass(
	VulkanDevice& Device,
	VkRenderPass RenderPass, 
	VkFramebuffer Framebuffer, 
	const VkRect2D& RenderArea,
	const std::vector<VkClearValue>& ClearValues,
	uint32 NumAttachments) 
	: Device(&Device)
	, RenderPass(RenderPass)
	, Framebuffer(Framebuffer)
	, RenderArea(RenderArea)
	, ClearValues(std::move(ClearValues))
	, NumAttachments(NumAttachments)
{
}

VulkanRenderPass::~VulkanRenderPass()
{
	if (Device)
	{
		vkDestroyFramebuffer(*Device, Framebuffer, nullptr);
	}
}

VulkanRenderPass::VulkanRenderPass(VulkanRenderPass&& Other)
	: Device(std::exchange(Other.Device, nullptr))
	, RenderPass(std::exchange(Other.RenderPass, VK_NULL_HANDLE))
	, Framebuffer(std::exchange(Other.Framebuffer, VK_NULL_HANDLE))
	, RenderArea(Other.RenderArea)
	, ClearValues(std::move(Other.ClearValues))
	, NumAttachments(Other.NumAttachments)
{
}

VulkanRenderPass& VulkanRenderPass::operator=(VulkanRenderPass&& Other)
{
	Device = std::exchange(Other.Device, nullptr);
	RenderPass = std::exchange(Other.RenderPass, VK_NULL_HANDLE);
	Framebuffer = std::exchange(Other.Framebuffer, VK_NULL_HANDLE);
	RenderArea = Other.RenderArea;
	ClearValues = std::move(Other.ClearValues);
	NumAttachments = Other.NumAttachments;
	return *this;
}