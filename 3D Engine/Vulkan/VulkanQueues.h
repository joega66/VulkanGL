#pragma once
#include <GPU/GPUResource.h>
#include <vulkan/vulkan.h>
#include <unordered_set>

class VulkanDevice;

class VulkanQueues
{
public:
	/** Finds the queue families for the physical device. */
	VulkanQueues(VulkanDevice& device);

	/** Gets the queues and creates the command pools. */
	void Create(VkDevice device);

	/** Checks if the queues are complete for rendering. */
	bool IsComplete() const;

	/** Gets the queue with these queue flags. */
	VkQueue GetQueue(VkQueueFlags queueFlags) const;

	/** Get a command pool that has the queue flags. */
	VkCommandPool GetCommandPool(VkQueueFlags queueFlags) const;

	inline int32 GetGraphicsIndex() const { return _GraphicsIndex; }
	inline int32 GetPresentIndex() const { return _PresentIndex; }
	inline VkQueue GetPresentQueue() const { return _PresentQueue; }

	/** Gets the unique families for logical device creation. */
	std::unordered_set<int32> GetUniqueFamilies() const;

private:
	std::vector<int32> _RequestedQueueFamilies;

	int32 _GraphicsIndex = -1;
	int32 _TransferIndex = -1;
	int32 _PresentIndex = -1;
	
	VkQueue _GraphicsQueue;
	VkCommandPool _GraphicsPool;
	VkQueue _TransferQueue;
	VkCommandPool _TransferPool;

	VkQueue _PresentQueue;
};