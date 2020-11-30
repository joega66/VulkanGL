#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"
#include <unordered_set>

const std::vector<const char*>& VulkanDevice::GetRequiredExtensions()
{
	static const std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
	};

	return deviceExtensions;
}

VulkanDevice::VulkanDevice(VulkanInstance& instance, VulkanPhysicalDevice& physicalDevice, std::vector<uint32> queueFamilyIndices)
	: _Instance(instance)
	, _PhysicalDevice(physicalDevice)
	, _VulkanCache(*this)
{
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

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

	const int32 graphicsIndex = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	int32 transferIndex = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);

	if (transferIndex == -1)
	{
		transferIndex = graphicsIndex;
	}

	std::unordered_set<int32> uniqueQueueFamilies = { graphicsIndex, transferIndex };
	for (auto queueFamilyIndex : queueFamilyIndices)
	{
		uniqueQueueFamilies.insert(queueFamilyIndex);
	}

	const float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueInfos;

	for (int32 queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;
		queueInfos.push_back(queueInfo);
	}

	const VkPhysicalDeviceFeatures physicalDeviceFeatures =
	{
		.geometryShader = true,
		.samplerAnisotropy = true,
		.vertexPipelineStoresAndAtomics = true,
		.fragmentStoresAndAtomics = true,
		.shaderStorageImageWriteWithoutFormat = true
	};

	VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
		.timelineSemaphore = true,
	};
	
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
		.pNext = &timelineSemaphoreFeatures,
		.shaderSampledImageArrayNonUniformIndexing = true,
		.shaderStorageImageArrayNonUniformIndexing = true,
		.descriptorBindingPartiallyBound = true,
		.descriptorBindingVariableDescriptorCount = true,
		.runtimeDescriptorArray = true,
	};
	
	const VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = 
	{ 
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &descriptorIndexingFeatures,
		.features = physicalDeviceFeatures,
	};
	
	VkDeviceCreateInfo deviceInfo = 
	{ 
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &physicalDeviceFeatures2,
		.queueCreateInfoCount = static_cast<uint32>(queueInfos.size()),
		.pQueueCreateInfos = queueInfos.data(),
		.enabledExtensionCount = static_cast<uint32>(GetRequiredExtensions().size()),
		.ppEnabledExtensionNames = GetRequiredExtensions().data(),
	};
	
	if (_Instance.UseValidationLayers())
	{
		deviceInfo.enabledLayerCount = static_cast<uint32>(_Instance.GetValidationLayers().size());
		deviceInfo.ppEnabledLayerNames = _Instance.GetValidationLayers().data();
	}
	else
	{
		deviceInfo.enabledLayerCount = 0;
	}

	vulkan(vkCreateDevice(_PhysicalDevice, &deviceInfo, nullptr, &_Device));

	_GraphicsQueue = VulkanQueue(_Device, graphicsIndex);
	_TransferQueue = VulkanQueue(_Device, transferIndex);
	
	const VmaAllocatorCreateInfo allocatorInfo =
	{
		.physicalDevice = _PhysicalDevice,
		.device = _Device,
		.instance = _Instance
	};

	vulkan(vmaCreateAllocator(&allocatorInfo, &_Allocator));
	
	_BindlessTextures = std::make_unique<VulkanBindlessDescriptors>(_Device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 65556);
	_BindlessImages = std::make_unique<VulkanBindlessDescriptors>(_Device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256);

	// Create the app's descriptor set layouts.
	auto& descriptorSetTypes = gpu::GetRegisteredDescriptorSetTypes();
	for (auto& descriptorSetType : descriptorSetTypes)
	{
		descriptorSetType.layout = CreateDescriptorSetLayout(descriptorSetType.bindings.size(), descriptorSetType.bindings.data());
	}

	// Create the app's descriptor pool.
	VkDescriptorPoolSize descriptorPoolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0 },
	};

	for (const auto& descriptorSetType : descriptorSetTypes)
	{
		for (const auto& binding : descriptorSetType.bindings)
		{
			descriptorPoolSizes[static_cast<uint32>(binding.descriptorType)].descriptorCount += binding.descriptorCount;
		}
	}

	const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = static_cast<uint32>(descriptorSetTypes.size()),
		.poolSizeCount = static_cast<uint32>(std::size(descriptorPoolSizes)),
		.pPoolSizes = descriptorPoolSizes,
	};

	vulkan(vkCreateDescriptorPool(_Device, &descriptorPoolCreateInfo, nullptr, &_DescriptorPool));

	// Create the app's descriptor sets.
	for (auto& descriptorSetType : descriptorSetTypes)
	{
		descriptorSetType.set = descriptorSetType.layout.CreateDescriptorSet();
	}
}