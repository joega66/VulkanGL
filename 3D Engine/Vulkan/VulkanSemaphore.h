#pragma once
#include <vulkan/vulkan.h>

namespace gpu
{
	/** A binary semaphore primarily for VkPresentInfoKHR which currently doesn't accept timeline semaphores. */
	class Semaphore
	{
		friend class VulkanDevice;
		Semaphore(VkDevice device);

	public:
		Semaphore() = default;
		Semaphore(Semaphore&& other);
		Semaphore& operator=(Semaphore&& other);
		~Semaphore();

		inline const VkSemaphore& Get() const { return _Semaphore; }
		
	private:
		VkDevice _Device;
		VkSemaphore _Semaphore = nullptr;

		void Destroy();
	};
}