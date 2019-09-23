#include "VulkanQueues.h"
#include "VulkanDevice.h"

static inline bool HasQueue(const VkQueueFamilyProperties& QueueFamily, VkQueueFlagBits QueueFlags)
{
	return QueueFamily.queueCount > 0 && QueueFamily.queueFlags & QueueFlags;
}

bool VulkanQueues::IsComplete() const
{
	return GraphicsIndex >= 0 && ComputeIndex >= 0 && TransferIndex >= 0 && PresentIndex >= 0;
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
		ComputeIndex,
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

	for (int32 QueueFamilyIndex = 0; QueueFamilyIndex < static_cast<int32>(QueueFamilies.size()); QueueFamilyIndex++)
	{
		const VkQueueFamilyProperties& QueueFamily = QueueFamilies[QueueFamilyIndex];

		if (HasQueue(QueueFamily, VK_QUEUE_GRAPHICS_BIT))
		{
			GraphicsIndex = QueueFamilyIndex;
		}

		if (HasQueue(QueueFamily, VK_QUEUE_COMPUTE_BIT))
		{
			ComputeIndex = QueueFamilyIndex;
		}

		if (HasQueue(QueueFamily, VK_QUEUE_TRANSFER_BIT))
		{
			TransferIndex = QueueFamilyIndex;
		}

		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(Device, QueueFamilyIndex, Surface, &PresentSupport);

		if (QueueFamilies[QueueFamilyIndex].queueCount > 0 && PresentSupport)
		{
			PresentIndex = QueueFamilyIndex;
		}

		if (IsComplete())
		{
			return;
		}
	}
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
	vkGetDeviceQueue(Device, ComputeIndex, 0, &ComputeQueue);
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

		if (ComputeIndex == QueueFamilyIndex)
		{
			ComputePool = CommandPool;
		}

		if (TransferIndex == QueueFamilyIndex)
		{
			TransferPool = CommandPool;
		}
	}
}