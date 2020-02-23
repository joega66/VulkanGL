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
	VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

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

	/** Render area. */
	VkRect2D RenderArea;

	/** Clear values to be used. */
	std::vector<VkClearValue> ClearValues;

	/** Number of color attachments. */
	uint32 NumAttachments;
};