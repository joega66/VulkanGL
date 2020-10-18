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
	
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = 
	{ 
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
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
	
	_BindlessTextures = std::make_shared<gpu::BindlessDescriptors>(_Device, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 65556);
	gBindlessTextures = _BindlessTextures;

	_BindlessSamplers = std::make_shared<gpu::BindlessDescriptors>(_Device, VK_DESCRIPTOR_TYPE_SAMPLER, 1024);
	gBindlessSamplers = _BindlessSamplers;

	_BindlessImages = std::make_shared<gpu::BindlessDescriptors>(_Device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256);
	gBindlessImages = _BindlessImages;
}