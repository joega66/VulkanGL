#include "VulkanRenderPass.h"
#include "VulkanDevice.h"

std::pair<VkRenderPass, VkFramebuffer> VulkanCache::GetRenderPass(const RenderPassDesc& rpDesc)
{
	std::size_t seed;

	for (const auto& colorAttachment : rpDesc.colorAttachments)
	{
		HashCombine(seed, colorAttachment.initialLayout);
		HashCombine(seed, colorAttachment.finalLayout);
		HashCombine(seed, colorAttachment.loadAction);
		HashCombine(seed, colorAttachment.storeAction);
	}

	HashCombine(seed, rpDesc.depthAttachment.initialLayout);
	HashCombine(seed, rpDesc.depthAttachment.finalLayout);
	HashCombine(seed, rpDesc.depthAttachment.loadAction);
	HashCombine(seed, rpDesc.depthAttachment.storeAction);

	VkRenderPass renderPass = VK_NULL_HANDLE;

	if (auto iter = RenderPassCache.find(seed); iter != RenderPassCache.end())
	{
		renderPass = iter->second;
	}

	if (renderPass == VK_NULL_HANDLE)
	{
		renderPass = CreateRenderPass(rpDesc);
		RenderPassCache.emplace(seed, renderPass);
	}

	const VkFramebuffer framebuffer = CreateFramebuffer(renderPass, rpDesc);
	return { renderPass, framebuffer };
}

static VkAttachmentLoadOp GetVulkanLoadOp(ELoadAction loadAction)
{
	if (loadAction == ELoadAction::Clear)
	{
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
	else if (loadAction == ELoadAction::Load)
	{
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	else // ELoadAction::DontCare
	{
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
}

static VkAttachmentStoreOp GetVulkanStoreOp(EStoreAction storeAction)
{
	if (storeAction == EStoreAction::Store)
	{
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
	else // EStoreAction::DontCare
	{
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
}

VkRenderPass VulkanCache::CreateRenderPass(const RenderPassDesc& rpDesc)
{
	std::vector<VkAttachmentDescription> descriptions;
	std::vector<VkAttachmentReference> colorRefs;
	colorRefs.reserve(rpDesc.colorAttachments.size());

	VkAttachmentReference depthRef = {};

	if (rpDesc.depthAttachment.image)
	{
		descriptions.reserve(rpDesc.colorAttachments.size() + 1);
	}
	else
	{
		descriptions.reserve(rpDesc.colorAttachments.size());
	}

	for (uint32 attachmentIndex = 0; attachmentIndex < rpDesc.colorAttachments.size(); attachmentIndex++)
	{
		const gpu::AttachmentView& colorAttachment = rpDesc.colorAttachments[attachmentIndex];
		const gpu::Image* image = colorAttachment.image;

		check(image && Any(image->GetUsage() & EImageUsage::Attachment), "Color target is invalid.");
		check(image->IsColor(), "Color target was not created in color format.");

		VkAttachmentDescription colorDescription = {};
		colorDescription.format = image->GetVulkanFormat();
		colorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		colorDescription.loadOp = GetVulkanLoadOp(colorAttachment.loadAction);
		colorDescription.storeOp = GetVulkanStoreOp(colorAttachment.storeAction);
		colorDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorDescription.initialLayout = VulkanImage::GetVulkanLayout(colorAttachment.initialLayout);
		colorDescription.finalLayout = VulkanImage::GetVulkanLayout(colorAttachment.finalLayout);
		descriptions.push_back(colorDescription);

		colorRefs.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	}

	if (rpDesc.depthAttachment.image)
	{
		const gpu::Image* depthImage = rpDesc.depthAttachment.image;

		check(depthImage && Any(depthImage->GetUsage() & EImageUsage::Attachment), "Depth target is invalid.");
		check(depthImage->IsDepth() || depthImage->IsStencil(), "Depth target was not created in a depth layout.");

		VkAttachmentLoadOp loadOp = GetVulkanLoadOp(rpDesc.depthAttachment.loadAction);
		VkAttachmentStoreOp storeOp = GetVulkanStoreOp(rpDesc.depthAttachment.storeAction);

		VkAttachmentDescription depthDescription = {};
		depthDescription.format = depthImage->GetVulkanFormat();
		depthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		depthDescription.loadOp = depthImage->IsDepth() ? loadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthDescription.storeOp = depthImage->IsDepth() ? storeOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthDescription.stencilLoadOp = depthImage->IsStencil() ? loadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthDescription.stencilStoreOp = depthImage->IsStencil() ? storeOp : VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthDescription.initialLayout = VulkanImage::GetVulkanLayout(rpDesc.depthAttachment.initialLayout);
		depthDescription.finalLayout = VulkanImage::GetVulkanLayout(rpDesc.depthAttachment.finalLayout);
		descriptions.push_back(depthDescription);

		depthRef.attachment = static_cast<uint32>(rpDesc.colorAttachments.size());
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorRefs.data();
	subpass.colorAttachmentCount = static_cast<uint32>(colorRefs.size());
	subpass.pDepthStencilAttachment = !rpDesc.depthAttachment.image ? nullptr : &depthRef;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.pAttachments = descriptions.data();
	renderPassInfo.attachmentCount = static_cast<uint32>(descriptions.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.pDependencies = dependencies.data();
	renderPassInfo.dependencyCount = static_cast<uint32>(dependencies.size());

	VkRenderPass renderPass;
	vulkan(vkCreateRenderPass(Device, &renderPassInfo, nullptr, &renderPass));

	return renderPass;
}

VkFramebuffer VulkanCache::CreateFramebuffer(VkRenderPass renderPass, const RenderPassDesc& rpDesc) const
{
	std::vector<VkImageView> attachmentViews;

	if (rpDesc.depthAttachment.image)
	{
		attachmentViews.reserve(rpDesc.colorAttachments.size() + 1);
	}
	else
	{
		attachmentViews.reserve(rpDesc.colorAttachments.size());
	}

	for (uint32 colorAttachmentIndex = 0; colorAttachmentIndex < rpDesc.colorAttachments.size(); colorAttachmentIndex++)
	{
		const gpu::Image* image = rpDesc.colorAttachments[colorAttachmentIndex].image;
		attachmentViews.push_back(image->GetImageView().GetHandle());
	}

	if (rpDesc.depthAttachment.image)
	{
		const gpu::Image* image = rpDesc.depthAttachment.image;
		attachmentViews.push_back(image->GetImageView().GetHandle());
	}

	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.pAttachments = attachmentViews.data();
	framebufferInfo.attachmentCount = static_cast<uint32>(attachmentViews.size());
	framebufferInfo.width = rpDesc.renderArea.extent.x;
	framebufferInfo.height = rpDesc.renderArea.extent.y;
	framebufferInfo.layers = 1;

	VkFramebuffer framebuffer;
	vulkan(vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &framebuffer));

	return framebuffer;
}

VulkanRenderPass::VulkanRenderPass(
	VulkanDevice& device,
	VkRenderPass renderPass, 
	VkFramebuffer framebuffer, 
	const VkRect2D& renderArea,
	const std::vector<VkClearValue>& clearValues,
	uint32 numAttachments) 
	: _Device(&device)
	, _RenderPass(renderPass)
	, _Framebuffer(framebuffer)
	, _RenderArea(renderArea)
	, _ClearValues(std::move(clearValues))
	, _NumAttachments(numAttachments)
{
}

VulkanRenderPass::~VulkanRenderPass()
{
	if (_Device)
	{
		vkDestroyFramebuffer(*_Device, _Framebuffer, nullptr);
	}
}

VulkanRenderPass::VulkanRenderPass(VulkanRenderPass&& other)
	: _Device(std::exchange(other._Device, nullptr))
	, _RenderPass(std::exchange(other._RenderPass, nullptr))
	, _Framebuffer(std::exchange(other._Framebuffer, nullptr))
	, _RenderArea(other._RenderArea)
	, _ClearValues(std::move(other._ClearValues))
	, _NumAttachments(other._NumAttachments)
{
}

VulkanRenderPass& VulkanRenderPass::operator=(VulkanRenderPass&& other)
{
	_Device = std::exchange(other._Device, nullptr);
	_RenderPass = std::exchange(other._RenderPass, nullptr);
	_Framebuffer = std::exchange(other._Framebuffer, nullptr);
	_RenderArea = other._RenderArea;
	_ClearValues = std::move(other._ClearValues);
	_NumAttachments = other._NumAttachments;
	return *this;
}

VulkanRenderPassView::VulkanRenderPassView(const VulkanRenderPass& renderPass)
	: _RenderPass(renderPass.GetRenderPass())
	, _NumAttachments(renderPass.GetNumAttachments())
{
}
