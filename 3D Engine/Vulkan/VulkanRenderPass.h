#pragma once
#include <Platform/Platform.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

class VulkanRenderPass
{
public:
	VulkanRenderPass() = default;

	VulkanRenderPass(
		VulkanDevice& device,
		VkRenderPass renderPass,
		VkFramebuffer framebuffer,
		const VkRect2D& renderArea,
		const std::vector<VkClearValue>& clearValues,
		uint32 numAttachments
	);
	~VulkanRenderPass();
	VulkanRenderPass(VulkanRenderPass&& other);
	VulkanRenderPass& operator=(VulkanRenderPass&& other);
	VulkanRenderPass(const VulkanRenderPass&) = delete;
	VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

	inline VkRenderPass GetRenderPass() const { return _RenderPass; }
	inline VkFramebuffer GetFramebuffer() const { return _Framebuffer; }
	inline const VkRect2D& GetRenderArea() const { return _RenderArea; }
	inline const std::vector<VkClearValue>& GetClearValues() const { return _ClearValues; }
	inline uint32 GetNumAttachments() const { return _NumAttachments; }

private:
	const VulkanDevice* _Device;

	/** The Vulkan render pass. */
	VkRenderPass _RenderPass;

	/** The Vulkan framebuffer. */
	VkFramebuffer _Framebuffer;

	/** Render area. */
	VkRect2D _RenderArea;

	/** Clear values to be used. */
	std::vector<VkClearValue> _ClearValues;

	/** Number of color attachments. */
	uint32 _NumAttachments;
};

/** 
  * Since the API couples the render pass and framebuffer for convenience, 
  * but PipelineStateDesc just needs the render pass, the API needs to offer
  * a view into the render pass.
  */
class VulkanRenderPassView
{
public:
	VulkanRenderPassView() = default;
	VulkanRenderPassView(const VulkanRenderPass& renderPass);
	inline VkRenderPass GetRenderPass() const { return _RenderPass; }
	inline uint32 GetNumAttachments() const { return _NumAttachments; }

	inline bool operator==(const VulkanRenderPassView& other) const { return _RenderPass == other._RenderPass; }

private:
	VkRenderPass _RenderPass = nullptr;
	uint32 _NumAttachments;
};