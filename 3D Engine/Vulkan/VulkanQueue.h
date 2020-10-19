#pragma once
#include <GPU/GPUResource.h>
#include <vulkan/vulkan.h>
#include <unordered_set>

namespace gpu
{ 
	class CommandList;
	class Buffer;
}

class VulkanQueue
{
public:
	VulkanQueue() = default;

	VulkanQueue(VkDevice device, int32 queueFamilyIndex);

	void Submit(const gpu::CommandList& cmdList);

	void WaitIdle(VkDevice device);

	void AddInFlightStagingBuffer(std::shared_ptr<gpu::Buffer> stagingBuffer);

	inline int32 GetQueueFamilyIndex() const { return _QueueFamilyIndex; }
	inline VkQueue GetQueue() const { return _Queue; }
	inline VkCommandPool GetCommandPool() const { return _CommandPool; }

private:
	int32 _QueueFamilyIndex = -1;

	VkQueue _Queue = VK_NULL_HANDLE;

	VkCommandPool _CommandPool = VK_NULL_HANDLE;

	std::vector<std::shared_ptr<gpu::Buffer>> _InFlightStagingBuffers;

	std::vector<VkCommandBuffer> _InFlightCmdBufs;

	VkSemaphore _TimelineSemaphore;

	uint64 _TimelineSemaphoreValue = 0;
};