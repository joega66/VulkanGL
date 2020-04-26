#pragma once
#include <Platform/Platform.h>
#include <vulkan/vulkan.h>
#include <unordered_set>

class VulkanQueues
{
public:
	/** Finds the queue families for the physical device. */
	VulkanQueues(VkPhysicalDevice Device);

	/** Gets the queues and creates the command pools. */
	void Create(VkDevice Device);

	/** Checks if the queues are complete for rendering. */
	bool IsComplete() const;

	/** Gets the queue with these queue flags. */
	VkQueue GetQueue(VkQueueFlags QueueFlags) const;

	/** Get a command pool that has the queue flags. */
	VkCommandPool GetCommandPool(VkQueueFlags QueueFlags) const;

	/** Get the graphics index. */
	inline int32 GetGraphicsIndex() const { return GraphicsIndex; }

	/** Gets the unique families for logical device creation. */
	std::unordered_set<int32> GetUniqueFamilies() const;

	/** Request that a queue family is created with the logical device. */
	void RequestQueueFamily(int32 QueueFamilyIndex);

private:
	std::vector<int32> RequestedQueueFamilies;

	int32 GraphicsIndex = -1;
	int32 TransferIndex = -1;
	
	VkQueue GraphicsQueue;
	VkCommandPool GraphicsPool;
	VkQueue TransferQueue;
	VkCommandPool TransferPool;
};