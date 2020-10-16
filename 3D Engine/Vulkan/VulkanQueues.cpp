#include "VulkanQueues.h"
#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanCommandList.h>

VulkanQueues::VulkanQueues(VulkanDevice& device)
{
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device.GetPhysicalDevice(), &queueFamilyCount, queueFamilies.data());

	auto GetQueueFamilyIndex = [&queueFamilies] (VkQueueFlags queueFlags)
	{
		int32 queueFamilyIndex = 0;

		for (auto& queueFamily : queueFamilies)
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

	_Queues[(std::size_t)EQueue::Graphics]._QueueFamilyIndex = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	_Queues[(std::size_t)EQueue::Transfer]._QueueFamilyIndex = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);

	if (device.GetSurface() != nullptr)
	{
		for (uint32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilies.size(); queueFamilyIndex++)
		{
			VkBool32 hasPresentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device.GetPhysicalDevice(), queueFamilyIndex, device.GetSurface(), &hasPresentSupport);

			if (hasPresentSupport)
			{
				_Queues[(std::size_t)EQueue::Present]._QueueFamilyIndex = queueFamilyIndex;
				break;
			}
		}

		check(_Queues[(std::size_t)EQueue::Present]._QueueFamilyIndex != -1, "No present family index found!!");
	}

	// If no transfer queue was found, use the graphics queue.
	if (_Queues[(std::size_t)EQueue::Transfer]._QueueFamilyIndex == -1)
	{
		_Queues[(std::size_t)EQueue::Transfer]._QueueFamilyIndex = _Queues[(std::size_t)EQueue::Graphics]._QueueFamilyIndex;
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
		if (queue._QueueFamilyIndex != -1)
		{
			vkGetDeviceQueue(device, queue._QueueFamilyIndex, 0, &queue._Queue);
		}
	}
	
	const std::unordered_set<int32> uniqueQueueFamilies = GetUniqueFamilies();

	for (auto queueFamilyIndex : uniqueQueueFamilies)
	{
		const VkCommandPool commandPool = CreateCommandPool(device, queueFamilyIndex);

		for (auto& queue : _Queues)
		{
			if (queue._QueueFamilyIndex == queueFamilyIndex)
			{
				queue._CommandPool = commandPool;
			}
		}
	}
}

std::unordered_set<int32> VulkanQueues::GetUniqueFamilies() const
{
	std::unordered_set<int32> uniqueQueueFamilies;

	for (auto& queue : _Queues)
	{
		if (queue._QueueFamilyIndex != -1)
		{
			uniqueQueueFamilies.insert(queue._QueueFamilyIndex);
		}
	}

	return uniqueQueueFamilies;
}

void VulkanQueue::Submit(const gpu::CommandList& cmdList)
{
	vulkan(vkEndCommandBuffer(cmdList._CommandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdList._CommandBuffer;

	vulkan(vkQueueSubmit(_Queue, 1, &submitInfo, VK_NULL_HANDLE));

	_InFlightCmdBufs.push_back(cmdList._CommandBuffer);
}

void VulkanQueue::WaitIdle(VkDevice device)
{
	if (_InFlightCmdBufs.size() > 0)
	{
		vulkan(vkQueueWaitIdle(_Queue));

		vkFreeCommandBuffers(device, _CommandPool, _InFlightCmdBufs.size(), _InFlightCmdBufs.data());

		_InFlightCmdBufs.clear();

		_InFlightStagingBuffers.clear();
	}
}

void VulkanQueue::AddInFlightStagingBuffer(std::shared_ptr<gpu::Buffer> stagingBuffer)
{
	_InFlightStagingBuffers.push_back(stagingBuffer);
}