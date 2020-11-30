#pragma once
#include <GPU/GPUResource.h>
#include <vulkan/vulkan.h>

class VulkanDevice;

namespace gpu
{
	class DescriptorSet
	{
	public:
		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet& operator=(const DescriptorSet&) = delete;

		DescriptorSet() = default;
		DescriptorSet(VkDescriptorSet descriptorSet);
		DescriptorSet(DescriptorSet&& other);
		DescriptorSet& operator=(DescriptorSet&& other);

		inline operator VkDescriptorSet() const { return _DescriptorSet; }
		inline const VkDescriptorSet& GetHandle() const { return _DescriptorSet; }

	private:
		VkDescriptorSet _DescriptorSet = nullptr;
	};

	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout() = default;

		DescriptorSetLayout(VulkanDevice& device, std::size_t numBindings, const DescriptorBinding* bindings);

		DescriptorSet CreateDescriptorSet();

		void UpdateDescriptorSet(const DescriptorSet& descriptorSet, const void* data);

		inline operator VkDescriptorSetLayout() const { return _DescriptorSetLayout; }

	private:
		VulkanDevice* _Device = nullptr;
		VkDescriptorSetLayout _DescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorUpdateTemplate _DescriptorUpdateTemplate = VK_NULL_HANDLE;
	};
};