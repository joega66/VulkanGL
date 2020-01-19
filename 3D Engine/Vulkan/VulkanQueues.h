#pragma once
#include <Platform/Platform.h>
#include <vulkan/vulkan.h>

class VulkanQueues
{
public:
	VulkanQueues() = default;

	/** Finds the queue families for the physical device. */
	void FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface);

	/** Gets the queues and creates the command pools. */
	void Create(VkDevice Device);

	bool IsComplete() const;

	VkQueue GetQueue(VkQueueFlags QueueFlags) const;

	VkCommandPool GetCommandPool(VkQueueFlags QueueFlags) const;

	inline int32 GetGraphicsIndex() const { return GraphicsIndex; }

	inline int32 GetPresentIndex() const { return PresentIndex; }

	inline VkQueue GetPresentQueue() const { return PresentQueue; }

	std::unordered_set<int32> GetUniqueFamilies() const;

private:
	int32 GraphicsIndex = -1;
	int32 TransferIndex = -1;
	int32 PresentIndex = -1;

	VkQueue GraphicsQueue;
	VkCommandPool GraphicsPool;
	VkQueue TransferQueue;
	VkCommandPool TransferPool;
	VkQueue PresentQueue;
};