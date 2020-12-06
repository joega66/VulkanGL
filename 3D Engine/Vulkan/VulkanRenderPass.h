#pragma once
#include <Platform/Platform.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

namespace gpu
{
	class RenderPass
	{
	public:
		RenderPass() = default;

		RenderPass(
			VulkanDevice& device,
			VkRenderPass renderPass,
			VkFramebuffer framebuffer,
			const VkRect2D& renderArea,
			const std::vector<VkClearValue>& clearValues,
			uint32 numAttachments
		);

		~RenderPass();
		RenderPass(RenderPass&& other);
		RenderPass& operator=(RenderPass&& other);
		RenderPass(const RenderPass&) = delete;
		RenderPass& operator=(const RenderPass&) = delete;

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
	  * but GraphicsPipelineDesc just needs the render pass, the API needs to offer
	  * a view into the render pass.
	  */
	class RenderPassView
	{
	public:
		RenderPassView() = default;
		RenderPassView(const RenderPass& renderPass);
		inline VkRenderPass GetRenderPass() const { return _RenderPass; }
		inline uint32 GetNumAttachments() const { return _NumAttachments; }

	private:
		VkRenderPass _RenderPass = nullptr;
		uint32 _NumAttachments;
	};
};