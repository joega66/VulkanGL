#pragma once
#include <vulkan/vulkan.h>

class VulkanDevice;

namespace gpu
{
	class Pipeline
	{
		friend class VulkanDevice;
	public:
		Pipeline() = default;
		Pipeline(std::shared_ptr<VkPipeline> pipeline, VkPipelineLayout pipelineLayout, VkPipelineBindPoint pipelineBindPoint);

		inline VkPipeline GetPipeline() const { return *_Pipeline; }
		inline VkPipelineLayout GetPipelineLayout() const { return _PipelineLayout; }
		inline VkPipelineBindPoint GetPipelineBindPoint() const { return _PipelineBindPoint; }

	private:
		std::shared_ptr<VkPipeline> _Pipeline = nullptr;
		VkPipelineLayout _PipelineLayout = VK_NULL_HANDLE;
		VkPipelineBindPoint _PipelineBindPoint;
	};
}