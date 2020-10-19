#include "VulkanQueue.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanCommandList.h"

VulkanQueue::VulkanQueue(VkDevice device, int32 queueFamilyIndex)
	: _QueueFamilyIndex(queueFamilyIndex)
{
	if (_QueueFamilyIndex != -1)
	{
		const VkCommandPoolCreateInfo commandPoolInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = static_cast<uint32>(_QueueFamilyIndex),
		};

		vkCreateCommandPool(device, &commandPoolInfo, nullptr, &_CommandPool);

		vkGetDeviceQueue(device, _QueueFamilyIndex, 0, &_Queue);

		const VkSemaphoreTypeCreateInfo semaphoreTypeInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = _TimelineSemaphoreValue
		};

		const VkSemaphoreCreateInfo semaphoreInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &semaphoreTypeInfo,
		};

		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_TimelineSemaphore);
	}
}

void VulkanQueue::Submit(const gpu::CommandList& cmdList)
{
	vkEndCommandBuffer(cmdList._CommandBuffer);

	const uint64 waitSemaphoreValue = _TimelineSemaphoreValue++;

	const VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.waitSemaphoreValueCount = 1,
		.pWaitSemaphoreValues = &waitSemaphoreValue,
		.signalSemaphoreValueCount = 1,
		.pSignalSemaphoreValues = &_TimelineSemaphoreValue
	};

	constexpr VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	const VkSubmitInfo submitInfo = 
	{ 
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = &timelineSemaphoreSubmitInfo,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &_TimelineSemaphore,
		.pWaitDstStageMask = &waitDstStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdList._CommandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &_TimelineSemaphore,
	};

	vkQueueSubmit(_Queue, 1, &submitInfo, VK_NULL_HANDLE);

	_InFlightCmdBufs.push_back(cmdList._CommandBuffer);
}

void VulkanQueue::WaitIdle(VkDevice device)
{
	if (_InFlightCmdBufs.size() > 0)
	{
		const VkSemaphoreWaitInfo semaphoreWaitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &_TimelineSemaphore,
			.pValues = &_TimelineSemaphoreValue
		};

		vkWaitSemaphores(device, &semaphoreWaitInfo, UINT64_MAX);

		vkFreeCommandBuffers(device, _CommandPool, static_cast<uint32>(_InFlightCmdBufs.size()), _InFlightCmdBufs.data());

		_InFlightCmdBufs.clear();

		_InFlightStagingBuffers.clear();
	}
}

void VulkanQueue::AddInFlightStagingBuffer(std::shared_ptr<gpu::Buffer> stagingBuffer)
{
	_InFlightStagingBuffers.push_back(stagingBuffer);
}