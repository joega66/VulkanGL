#include "VulkanQueue.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanCommandList.h"
#include "VulkanSemaphore.h"

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

void VulkanQueue::Submit(
	const gpu::CommandList& cmdList,
	const gpu::Semaphore& waitSemaphore,
	const gpu::Semaphore& signalSemaphore)
{
	vkEndCommandBuffer(cmdList._CommandBuffer);

	const auto waitSemaphoreValues = waitSemaphore.Get() == VK_NULL_HANDLE ? 
		std::vector{ _TimelineSemaphoreValue } : std::vector{ _TimelineSemaphoreValue, 0llu };

	const auto waitSemaphores = waitSemaphore.Get() == VK_NULL_HANDLE ?
		std::vector{ _TimelineSemaphore } : std::vector{ _TimelineSemaphore, waitSemaphore.Get() };

	const auto signalSemaphoreValues = signalSemaphore.Get() == VK_NULL_HANDLE ?
		std::vector{ ++_TimelineSemaphoreValue } : std::vector{ ++_TimelineSemaphoreValue, 0llu };

	const auto signalSemaphores = signalSemaphore.Get() == VK_NULL_HANDLE ?
		std::vector{ _TimelineSemaphore } : std::vector{ _TimelineSemaphore, signalSemaphore.Get() };

	const VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.waitSemaphoreValueCount = static_cast<uint32>(waitSemaphoreValues.size()),
		.pWaitSemaphoreValues = waitSemaphoreValues.data(),
		.signalSemaphoreValueCount = static_cast<uint32>(signalSemaphoreValues.size()),
		.pSignalSemaphoreValues = signalSemaphoreValues.data()
	};

	const std::vector<VkPipelineStageFlags> waitDstStage(waitSemaphores.size(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

	const VkSubmitInfo submitInfo = 
	{ 
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = &timelineSemaphoreSubmitInfo,
		.waitSemaphoreCount = static_cast<uint32>(waitSemaphores.size()),
		.pWaitSemaphores = waitSemaphores.data(),
		.pWaitDstStageMask = waitDstStage.data(),
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdList._CommandBuffer,
		.signalSemaphoreCount = static_cast<uint32>(signalSemaphores.size()),
		.pSignalSemaphores = signalSemaphores.data(),
	};

	vkQueueSubmit(_Queue, 1, &submitInfo, VK_NULL_HANDLE);

	_InFlightCmdBufs.push_back(cmdList._CommandBuffer);
}

void VulkanQueue::WaitSemaphores(VkDevice device)
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

		GiveUpInFlightResources(device);
	}
}

void VulkanQueue::AddInFlightStagingBuffer(std::shared_ptr<gpu::Buffer> stagingBuffer)
{
	_InFlightStagingBufs.push_back(stagingBuffer);
}

void VulkanQueue::GiveUpInFlightResources(VkDevice device)
{
	vkFreeCommandBuffers(device, _CommandPool, static_cast<uint32>(_InFlightCmdBufs.size()), _InFlightCmdBufs.data());

	_InFlightCmdBufs.clear();

	_InFlightStagingBufs.clear();
}