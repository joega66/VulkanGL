#include "VulkanQueues.h"
#include <Vulkan/VulkanDevice.h>

VulkanQueues::VulkanQueues(VulkanDevice& device)
{
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueFamilyCount, queueFamilies.data());

	auto GetQueueFamilyIndex = [&] (VkQueueFlags queueFlags)
	{
		int32 queueFamilyIndex = 0;

		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & queueFlags)
			{
				return queueFamilyIndex;
			}

			queueFamilyIndex++;
		}

		return -1;
	};

	_GraphicsIndex = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	_TransferIndex = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);

	if (device.GetSurface() != nullptr)
	{
		for (uint32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilies.size(); queueFamilyIndex++)
		{
			const VkQueueFamilyProperties& queueFamily = queueFamilies[queueFamilyIndex];

			VkBool32 hasPresentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device.GetPhysicalDevice(), queueFamilyIndex, device.GetSurface(), &hasPresentSupport);

			if (hasPresentSupport)
			{
				_PresentIndex = queueFamilyIndex;
				break;
			}
		}

		check(_PresentIndex != -1, "No present family index found!!");
	}

	// If no transfer queue was found, use the graphics queue.
	if (_TransferIndex == -1)
	{
		_TransferIndex = _GraphicsIndex;
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
	vkCreateCommandPool(Device, &PoolInfo, nullptr, &CommandPool);

	return CommandPool;
}

void VulkanQueues::Create(VkDevice Device)
{
	vkGetDeviceQueue(Device, _GraphicsIndex, 0, &_GraphicsQueue);
	vkGetDeviceQueue(Device, _TransferIndex, 0, &_TransferQueue);
	vkGetDeviceQueue(Device, _PresentIndex, 0, &_PresentQueue);
	
	const std::unordered_set<int32> UniqueQueueFamilies = GetUniqueFamilies();

	for (auto QueueFamilyIndex : UniqueQueueFamilies)
	{
		VkCommandPool CommandPool = CreateCommandPool(Device, QueueFamilyIndex);

		if (_GraphicsIndex == QueueFamilyIndex)
		{
			_GraphicsPool = CommandPool;
		}

		if (_TransferIndex == QueueFamilyIndex)
		{
			_TransferPool = CommandPool;
		}
	}
}

bool VulkanQueues::IsComplete() const
{
	return _GraphicsIndex >= 0 && _TransferIndex >= 0;
}

VkQueue VulkanQueues::GetQueue(VkQueueFlags QueueFlags) const
{
	if (QueueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		return _TransferQueue;
	}
	else
	{
		return _GraphicsQueue;
	}
}

VkCommandPool VulkanQueues::GetCommandPool(VkQueueFlags QueueFlags) const
{
	if (QueueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		return _TransferPool;
	}
	else
	{
		return _GraphicsPool;
	}
}

std::unordered_set<int32> VulkanQueues::GetUniqueFamilies() const
{
	std::unordered_set<int32> UniqueQueueFamilies =
	{
		_GraphicsIndex,
		_TransferIndex,
		_PresentIndex,
	};

	return UniqueQueueFamilies;
}