#pragma once
#include <Platform/Platform.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanRenderPass
{
public:
	VulkanRenderPass() = default;

	VulkanRenderPass(
		VulkanDevice& Device,
		VkRenderPass RenderPass,
		VkFramebuffer Framebuffer,
		const VkRect2D& RenderArea,
		const std::vector<VkClearValue>& ClearValues,
		uint32 NumAttachments
	);
	~VulkanRenderPass();
	VulkanRenderPass(VulkanRenderPass&& Other);
	VulkanRenderPass& operator=(VulkanRenderPass&& Other);
	VulkanRenderPass(const VulkanRenderPass&) = delete;

	inline VkRenderPass GetRenderPass() const { return RenderPass; }
	inline VkFramebuffer GetFramebuffer() const { return Framebuffer; }
	inline const VkRect2D& GetRenderArea() const { return RenderArea; }
	inline const std::vector<VkClearValue>& GetClearValues() const { return ClearValues; }
	inline uint32 GetNumAttachments() const { return NumAttachments; }

private:
	const VulkanDevice* Device;

	/** The Vulkan render pass. */
	VkRenderPass RenderPass;

	/** The Vulkan framebuffer. */
	VkFramebuffer Framebuffer;

	/** 
	  * Yeah, render area is in RenderPassBeginInfo, but I don't allow 
	  * specifying a render area smaller than the framebuffer because it's really not
	  * optimal on mobile.
	  */
	VkRect2D RenderArea;

	/** Clear values to be used. @todo Make these public in RenderPass. */
	std::vector<VkClearValue> ClearValues;

	/** Number of color attachments. */
	uint32 NumAttachments;
};