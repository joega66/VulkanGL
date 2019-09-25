#include "VulkanQueues.h"
#include "VulkanDevice.h"

static inline bool HasQueue(const VkQueueFamilyProperties& QueueFamily, VkQueueFlagBits QueueFlags)
{
	return QueueFamily.queueCount > 0 && QueueFamily.queueFlags & QueueFlags;
}

bool VulkanQueues::IsComplete() const
{
	return GraphicsIndex >= 0 && TransferIndex >= 0 && PresentIndex >= 0;
}

VkQueue VulkanQueues::GetQueue(VkQueueFlags QueueFlags) const
{
	if (QueueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		return TransferQueue;
	}
	else
	{
		return GraphicsQueue;
	}
}

VkCommandPool VulkanQueues::GetCommandPool(VkQueueFlags QueueFlags) const
{
	if (QueueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		return TransferPool;
	}
	else
	{
		return GraphicsPool;
	}
}

std::unordered_set<int32> VulkanQueues::GetUniqueFamilies() const
{
	const std::unordered_set<int32> UniqueQueueFamilies =
	{
		GraphicsIndex,
		TransferIndex,
		PresentIndex
	};
	return UniqueQueueFamilies;
}

void VulkanQueues::FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface)
{
	uint32 QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

	// Find a queue family with graphics, compute, and present.
	for (int32 QueueFamilyIndex = 0; QueueFamilyIndex < static_cast<int32>(QueueFamilies.size()); QueueFamilyIndex++)
	{
		const VkQueueFamilyProperties& QueueFamily = QueueFamilies[QueueFamilyIndex];

		VkBool32 HasPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(Device, QueueFamilyIndex, Surface, &HasPresentSupport);

		if (HasQueue(QueueFamily, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) && HasPresentSupport)
		{
			GraphicsIndex = QueueFamilyIndex;
			PresentIndex = QueueFamilyIndex;
			break;
		}
	}

	// Check if a separate transfer queue is available.
	for (int32 QueueFamilyIndex = 0; QueueFamilyIndex < static_cast<int32>(QueueFamilies.size()); QueueFamilyIndex++)
	{
		const VkQueueFamilyProperties& QueueFamily = QueueFamilies[QueueFamilyIndex];
		if (QueueFamilyIndex != GraphicsIndex && HasQueue(QueueFamily, VK_QUEUE_TRANSFER_BIT))
		{
			TransferIndex = QueueFamilyIndex;
			break;
		}
	}

	// If none was found, use the graphics queue for transfers.
	if (TransferIndex == -1)
	{
		TransferIndex = GraphicsIndex;
	}

	if (TransferIndex != GraphicsIndex)
	{
		LOG("Using separate transfer and graphics queues.");
	}

	check(IsComplete(), "The queue families are not complete.");
}

static VkCommandPool CreateCommandPool(
	VkDevice Device,
	uint32 QueueFamilyIndex)
{
	VkCommandPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	PoolInfo.queueFamilyIndex = QueueFamilyIndex;
	PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool CommandPool;
	vulkan(vkCreateCommandPool(Device, &PoolInfo, nullptr, &CommandPool));

	return CommandPool;
}

void VulkanQueues::Create(VkDevice Device)
{
	vkGetDeviceQueue(Device, GraphicsIndex, 0, &GraphicsQueue);
	vkGetDeviceQueue(Device, TransferIndex, 0, &TransferQueue);
	vkGetDeviceQueue(Device, PresentIndex, 0, &PresentQueue);

	const std::unordered_set<int32> UniqueQueueFamilies = GetUniqueFamilies();
	std::vector<VkCommandPool> CommandPools;

	for (auto QueueFamilyIndex : UniqueQueueFamilies)
	{
		VkCommandPool CommandPool = CreateCommandPool(Device, QueueFamilyIndex);

		if (GraphicsIndex == QueueFamilyIndex)
		{
			GraphicsPool = CommandPool;
		}

		if (TransferIndex == QueueFamilyIndex)
		{
			TransferPool = CommandPool;
		}
	}
}