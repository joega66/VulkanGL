#include "VulkanRenderPass.h"
#include "VulkanDevice.h"

std::pair<VkRenderPass, VkFramebuffer> VulkanDevice::GetOrCreateRenderPass(const RenderPassDesc& rpDesc)
{
	Crc crc = 0;
	
	for (const auto& colorAttachment : rpDesc.colorAttachments)
	{
		Platform::crc32_u32(crc, &colorAttachment.initialLayout, sizeof(colorAttachment.initialLayout));
		Platform::crc32_u32(crc, &colorAttachment.finalLayout, sizeof(colorAttachment.finalLayout));
		Platform::crc32_u32(crc, &colorAttachment.loadAction, sizeof(colorAttachment.loadAction));
		Platform::crc32_u32(crc, &colorAttachment.storeAction, sizeof(colorAttachment.storeAction));
	}

	Platform::crc32_u32(crc, &rpDesc.depthAttachment.initialLayout, sizeof(rpDesc.depthAttachment.initialLayout));
	Platform::crc32_u32(crc, &rpDesc.depthAttachment.finalLayout, sizeof(rpDesc.depthAttachment.finalLayout));
	Platform::crc32_u32(crc, &rpDesc.depthAttachment.loadAction, sizeof(rpDesc.depthAttachment.loadAction));
	Platform::crc32_u32(crc, &rpDesc.depthAttachment.storeAction, sizeof(rpDesc.depthAttachment.storeAction));

	VkRenderPass renderPass = VK_NULL_HANDLE;

	if (auto iter = _RenderPassCache.find(crc); iter != _RenderPassCache.end())
	{
		renderPass = iter->second;
	}

	if (renderPass == VK_NULL_HANDLE)
	{
		renderPass = CreateRenderPass(_Device, rpDesc);
		_RenderPassCache.emplace(crc, renderPass);
	}

	const VkFramebuffer framebuffer = CreateFramebuffer(renderPass, rpDesc);
	return { renderPass, framebuffer };
}

static VkAttachmentLoadOp GetVulkanLoadOp(ELoadAction loadAction)
{
	static const VkAttachmentLoadOp loadOps[] =
	{
		VK_ATTACHMENT_LOAD_OP_LOAD,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	};

	return loadOps[static_cast<uint32>(loadAction)];
}

static VkAttachmentStoreOp GetVulkanStoreOp(EStoreAction storeAction)
{
	static const VkAttachmentStoreOp storeOps[] =
	{
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
	};

	return storeOps[static_cast<uint32>(storeAction)];
}

VkRenderPass VulkanDevice::CreateRenderPass(VkDevice device, const RenderPassDesc& rpDesc)
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
		const AttachmentView& colorAttachment = rpDesc.colorAttachments[attachmentIndex];
		const gpu::Image* image = colorAttachment.image;

		check(image && Any(image->GetUsage() & EImageUsage::Attachment), "Color target is invalid.");
		check(image->IsColor(), "Color target was not created in color format.");

		descriptions.push_back({
			.format = image->GetVulkanFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = GetVulkanLoadOp(colorAttachment.loadAction),
			.storeOp = GetVulkanStoreOp(colorAttachment.storeAction),
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = gpu::Image::GetLayout(colorAttachment.initialLayout),
			.finalLayout = gpu::Image::GetLayout(colorAttachment.finalLayout),
		});

		colorRefs.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	}

	if (rpDesc.depthAttachment.image)
	{
		const gpu::Image* depthImage = rpDesc.depthAttachment.image;

		check(depthImage && Any(depthImage->GetUsage() & EImageUsage::Attachment), "Depth target is invalid.");
		check(depthImage->IsDepth() || depthImage->IsStencil(), "Depth target was not created in a depth layout.");

		const VkAttachmentLoadOp loadOp = GetVulkanLoadOp(rpDesc.depthAttachment.loadAction);
		const VkAttachmentStoreOp storeOp = GetVulkanStoreOp(rpDesc.depthAttachment.storeAction);

		descriptions.push_back({
			.format = depthImage->GetVulkanFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = depthImage->IsDepth() ? loadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = depthImage->IsDepth() ? storeOp : VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = depthImage->IsStencil() ? loadOp : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = depthImage->IsStencil() ? storeOp : VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = gpu::Image::GetLayout(rpDesc.depthAttachment.initialLayout),
			.finalLayout = gpu::Image::GetLayout(rpDesc.depthAttachment.finalLayout),
		});

		depthRef.attachment = static_cast<uint32>(rpDesc.colorAttachments.size());
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	const VkSubpassDescription subpass = 
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = static_cast<uint32>(colorRefs.size()),
		.pColorAttachments = colorRefs.data(),
		.pDepthStencilAttachment = !rpDesc.depthAttachment.image ? nullptr : &depthRef,
	};
	
	const VkSubpassDependency dependencies[] =
	{
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VulkanDevice::GetPipelineStageFlags(rpDesc.srcStageMask),
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VulkanDevice::GetAccessFlags(rpDesc.srcAccessMask),
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		},
		{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VulkanDevice::GetPipelineStageFlags(rpDesc.dstStageMask),
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VulkanDevice::GetAccessFlags(rpDesc.dstAccessMask),
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		}
	};

	const VkRenderPassCreateInfo renderPassCreateInfo = 
	{ 
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32>(descriptions.size()),
		.pAttachments = descriptions.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32>(std::size(dependencies)),
		.pDependencies = dependencies,
	};
	
	VkRenderPass renderPass;
	vulkan(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass));

	return renderPass;
}

VkFramebuffer VulkanDevice::CreateFramebuffer(VkRenderPass renderPass, const RenderPassDesc& rpDesc) const
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
		attachmentViews.push_back(image->GetImageView());
	}

	if (rpDesc.depthAttachment.image)
	{
		const gpu::Image* image = rpDesc.depthAttachment.image;
		attachmentViews.push_back(image->GetImageView());
	}

	const VkFramebufferCreateInfo framebufferCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = static_cast<uint32>(attachmentViews.size()),
		.pAttachments = attachmentViews.data(),
		.width = rpDesc.renderArea.extent.x,
		.height = rpDesc.renderArea.extent.y,
		.layers = 1,
	};
	
	VkFramebuffer framebuffer;
	vulkan(vkCreateFramebuffer(_Device, &framebufferCreateInfo, nullptr, &framebuffer));

	return framebuffer;
}

namespace gpu
{
	RenderPass::RenderPass(
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
		, _ClearValues(clearValues)
		, _NumAttachments(numAttachments)
	{
	}

	RenderPass::~RenderPass()
	{
		if (_Device)
		{
			vkDestroyFramebuffer(*_Device, _Framebuffer, nullptr);
		}
	}

	RenderPass::RenderPass(RenderPass&& other)
		: _Device(std::exchange(other._Device, nullptr))
		, _RenderPass(std::exchange(other._RenderPass, nullptr))
		, _Framebuffer(std::exchange(other._Framebuffer, nullptr))
		, _RenderArea(other._RenderArea)
		, _ClearValues(std::move(other._ClearValues))
		, _NumAttachments(other._NumAttachments)
	{
	}

	RenderPass& RenderPass::operator=(RenderPass&& other)
	{
		_Device = std::exchange(other._Device, nullptr);
		_RenderPass = std::exchange(other._RenderPass, nullptr);
		_Framebuffer = std::exchange(other._Framebuffer, nullptr);
		_RenderArea = other._RenderArea;
		_ClearValues = std::move(other._ClearValues);
		_NumAttachments = other._NumAttachments;
		return *this;
	}

	RenderPassView::RenderPassView(const RenderPass& renderPass)
		: _RenderPass(renderPass.GetRenderPass())
		, _NumAttachments(renderPass.GetNumAttachments())
	{
	}
};