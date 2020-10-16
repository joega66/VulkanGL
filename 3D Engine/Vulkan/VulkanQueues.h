#pragma once
#include <GPU/GPUResource.h>
#include <vulkan/vulkan.h>
#include <unordered_set>

class VulkanDevice;

namespace gpu 
{ 
	class CommandList;
	class Buffer;
}

class VulkanQueue
{
	friend class VulkanQueues;
public:
	void Submit(const gpu::CommandList& cmdList);

	void WaitIdle(VkDevice device);

	void AddInFlightStagingBuffer(std::shared_ptr<gpu::Buffer> stagingBuffer);

	inline int32 GetQueueFamilyIndex() const { return _QueueFamilyIndex; }
	inline VkQueue GetQueue() const { return _Queue; }
	inline VkCommandPool GetCommandPool() const { return _CommandPool; }

private:
	int32 _QueueFamilyIndex = -1;

	VkQueue _Queue;

	VkCommandPool _CommandPool;

	std::vector<std::shared_ptr<gpu::Buffer>> _InFlightStagingBuffers;

	std::vector<VkCommandBuffer> _InFlightCmdBufs;
};

class VulkanQueues
{
public:
	/** Finds the queue families for the physical device. */
	VulkanQueues(VulkanDevice& device);

	/** Gets the queues and creates the command pools. */
	void Create(VkDevice device);

	/** Get this queue. */
	VulkanQueue& GetQueue(EQueue queue) { return _Queues[static_cast<std::size_t>(queue)]; }

	/** Gets the unique families for logical device creation. */
	std::unordered_set<int32> GetUniqueFamilies() const;

private:
	VulkanQueue _Queues[(std::size_t)EQueue::Num];

};