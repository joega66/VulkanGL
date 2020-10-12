#pragma once
#include <GPU/GPUResource.h>
#include <vulkan/vulkan.h>
#include <unordered_set>

class VulkanDevice;

struct VulkanQueue
{
	int32 queueFamilyIndex = -1;
	VkQueue queue;
	VkCommandPool commandPool;
};

class VulkanQueues
{
public:
	/** Finds the queue families for the physical device. */
	VulkanQueues(VulkanDevice& device);

	/** Gets the queues and creates the command pools. */
	void Create(VkDevice device);

	/** Get this queue. */
	const VulkanQueue& GetQueue(EQueue queue) const { return _Queues[static_cast<std::size_t>(queue)]; }

	/** Gets the unique families for logical device creation. */
	std::unordered_set<int32> GetUniqueFamilies() const;

private:
	VulkanQueue _Queues[(std::size_t)EQueue::Num];

};