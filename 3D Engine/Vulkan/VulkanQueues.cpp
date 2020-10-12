#include "VulkanQueues.h"
#include <Vulkan/VulkanDevice.h>

VulkanQueues::VulkanQueues(VulkanDevice& device)
{
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueFamilyCount, queueFamilies.data());

	std::vector<VkQueueFamilyProperties> unusedQueueFamilies = queueFamilies;

	auto GetQueueFamilyIndex = [&] (VkQueueFlags queueFlags)
	{
		int32 queueFamilyIndex = 0;

		for (auto& queueFamily : unusedQueueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & queueFlags)
			{
				queueFamily.queueCount = 0;
				return queueFamilyIndex;
			}

			queueFamilyIndex++;
		}

		return -1;
	};

	_Queues[(std::size_t)EQueue::Graphics].queueFamilyIndex = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	_Queues[(std::size_t)EQueue::Transfer].queueFamilyIndex = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);

	if (device.GetSurface() != nullptr)
	{
		for (uint32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilies.size(); queueFamilyIndex++)
		{
			const VkQueueFamilyProperties& queueFamily = queueFamilies[queueFamilyIndex];

			VkBool32 hasPresentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device.GetPhysicalDevice(), queueFamilyIndex, device.GetSurface(), &hasPresentSupport);

			if (hasPresentSupport)
			{
				_Queues[(std::size_t)EQueue::Present].queueFamilyIndex = queueFamilyIndex;
				break;
			}
		}

		check(_Queues[(std::size_t)EQueue::Present].queueFamilyIndex != -1, "No present family index found!!");
	}

	// If no transfer queue was found, use the graphics queue.
	if (_Queues[(std::size_t)EQueue::Transfer].queueFamilyIndex == -1)
	{
		_Queues[(std::size_t)EQueue::Transfer].queueFamilyIndex = _Queues[(std::size_t)EQueue::Graphics].queueFamilyIndex;
	}
}

static VkCommandPool CreateCommandPool(
	VkDevice device,
	uint32 queueFamilyIndex)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool commandPool;
	vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

	return commandPool;
}

void VulkanQueues::Create(VkDevice device)
{
	for (auto& queue : _Queues)
	{
		if (queue.queueFamilyIndex != -1)
		{
			vkGetDeviceQueue(device, queue.queueFamilyIndex, 0, &queue.queue);
		}
	}
	
	const std::unordered_set<int32> uniqueQueueFamilies = GetUniqueFamilies();

	for (auto queueFamilyIndex : uniqueQueueFamilies)
	{
		const VkCommandPool commandPool = CreateCommandPool(device, queueFamilyIndex);

		for (auto& queue : _Queues)
		{
			if (queue.queueFamilyIndex == queueFamilyIndex)
			{
				queue.commandPool = commandPool;
			}
		}
	}
}

std::unordered_set<int32> VulkanQueues::GetUniqueFamilies() const
{
	std::unordered_set<int32> uniqueQueueFamilies;

	for (auto& queue : _Queues)
	{
		if (queue.queueFamilyIndex != -1)
		{
			uniqueQueueFamilies.insert(queue.queueFamilyIndex);
		}
	}

	return uniqueQueueFamilies;
}