#include "VulkanSemaphore.h"
#include <utility>

gpu::Semaphore::Semaphore(VkDevice device)
	: _Device(device)
{
	const VkSemaphoreCreateInfo semaphoreInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Semaphore);
}

gpu::Semaphore::Semaphore(Semaphore&& other)
	: _Device(other._Device)
	, _Semaphore(std::exchange(other._Semaphore, nullptr))
{
}

gpu::Semaphore& gpu::Semaphore::operator=(Semaphore&& other)
{
	Destroy();
	_Device = other._Device;
	_Semaphore = std::exchange(other._Semaphore, nullptr);
	return *this;
}

gpu::Semaphore::~Semaphore()
{
	Destroy();
}

void gpu::Semaphore::Destroy()
{
	if (_Semaphore != nullptr)
	{
		vkDestroySemaphore(_Device, _Semaphore, nullptr);

		_Semaphore = nullptr;
	}
}