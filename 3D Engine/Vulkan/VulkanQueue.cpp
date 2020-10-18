#include "VulkanQueue.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanCommandList.h"

VulkanQueue::VulkanQueue(VkDevice device, int32 queueFamilyIndex)
	: _QueueFamilyIndex(queueFamilyIndex)
{
	if (_QueueFamilyIndex != -1)
	{
		VkCommandPoolCreateInfo commandPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		commandPoolInfo.queueFamilyIndex = _QueueFamilyIndex;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		vkCreateCommandPool(device, &commandPoolInfo, nullptr, &_CommandPool);

		vkGetDeviceQueue(device, _QueueFamilyIndex, 0, &_Queue);
	}
}

void VulkanQueue::Submit(const gpu::CommandList& cmdList)
{
	vkEndCommandBuffer(cmdList._CommandBuffer);

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdList._CommandBuffer;

	vkQueueSubmit(_Queue, 1, &submitInfo, VK_NULL_HANDLE);

	_InFlightCmdBufs.push_back(cmdList._CommandBuffer);
}

void VulkanQueue::WaitIdle(VkDevice device)
{
	if (_InFlightCmdBufs.size() > 0)
	{
		vkQueueWaitIdle(_Queue);

		vkFreeCommandBuffers(device, _CommandPool, _InFlightCmdBufs.size(), _InFlightCmdBufs.data());

		_InFlightCmdBufs.clear();

		_InFlightStagingBuffers.clear();
	}
}

void VulkanQueue::AddInFlightStagingBuffer(std::shared_ptr<gpu::Buffer> stagingBuffer)
{
	_InFlightStagingBuffers.push_back(stagingBuffer);
}