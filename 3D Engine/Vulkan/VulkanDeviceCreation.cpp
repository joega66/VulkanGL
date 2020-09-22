#include "VulkanDevice.h"

#if _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <unordered_set>

static const std::vector<const char*> gValidationLayers =
{
	"VK_LAYER_LUNARG_standard_validation"
};

static const std::vector<const char*> gDeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
};

static bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
	uint32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		if (![] (const char* layerName, const std::vector<VkLayerProperties>& availableLayers)
		{
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					return true;
				}
			}

			return false;
		} (layerName, availableLayers))
		{
			return false;
		}
	}

	return true;
}

static VkInstance CreateInstance(const std::vector<const char*>& validationLayers, bool useValidationLayers)
{
	if (useValidationLayers && !CheckValidationLayerSupport(validationLayers))
	{
		fail("Validation layers requested, but are unavailable.");
	}

	VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	applicationInfo.pApplicationName = "Vulkan Engine";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "No Engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_2;

	std::vector<const char*> extensions;

	if (useValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if _WIN32
	extensions.push_back("VK_KHR_win32_surface");
#endif

	VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceInfo.pApplicationInfo = &applicationInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	if (useValidationLayers)
	{
		instanceInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
	}

	VkInstance instance;
	vulkan(vkCreateInstance(&instanceInfo, nullptr, &instance));

	return instance;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
	uint64 obj, size_t location, int32 code, const char* layerPrefix, const char* msg, void* userData)
{
	LOG("Validation layer: %s\nLayer prefix: %s", msg, layerPrefix);
	return false;
}

static VkDebugReportCallbackEXT CreateDebugReportCallback(VkInstance instance, bool useValidationLayers)
{
	if (useValidationLayers)
	{
		// Create Vulkan debug callback
		VkDebugReportCallbackCreateInfoEXT debugCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		debugCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debugCallbackInfo.pfnCallback = DebugCallback;

		auto createDebugReportCallbackEXT = [] (VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
		{
			auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
			if (func)
			{
				return func(instance, pCreateInfo, pAllocator, pCallback);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		};

		VkDebugReportCallbackEXT debugReportCallback;
		vulkan(createDebugReportCallbackEXT(instance, &debugCallbackInfo, nullptr, &debugReportCallback));
	}

	return 0;
}

static bool AllDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions)
{
	uint32 extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

/** Select a Vulkan-capable physical device. */
static VkPhysicalDevice SelectPhysicalDevice(VkInstance instance, const std::vector<const char*>& deviceExtensions)
{
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	check(deviceCount != 0, "Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> availablePhysicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	for (const auto& availablePhysicalDevice : availablePhysicalDevices)
	{
		if (AllDeviceExtensionsSupported(availablePhysicalDevice, deviceExtensions))
		{
			physicalDevice = availablePhysicalDevice;
			break;
		}
	}

	check(physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU.");

	return physicalDevice;
}

static VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	return properties;
}

static VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	return features;
}

static VkSurfaceKHR CreateSurface(void* windowHandle, VkInstance instance)
{
	if (windowHandle == nullptr)
	{
		return nullptr;
	}

	VkSurfaceKHR surface;

#if _WIN32
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>
		(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));

	check(vkCreateWin32SurfaceKHR, "Failed to get proc address vkCreateWin32SurfaceKHR");

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
	surfaceCreateInfo.hwnd = static_cast<HWND>(windowHandle);

	vulkan(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));
#endif

	return surface;
}

VulkanDevice::VulkanDevice(const DeviceDesc& deviceDesc)
	: _Instance(CreateInstance(gValidationLayers, deviceDesc.enableValidationLayers))
	, _DebugReportCallback(CreateDebugReportCallback(_Instance, deviceDesc.enableValidationLayers))
	, _PhysicalDevice(SelectPhysicalDevice(_Instance, gDeviceExtensions))
	, _Surface(CreateSurface(deviceDesc.windowHandle, _Instance))
	, _Queues(*this)
	, _PhysicalDeviceProperties(GetPhysicalDeviceProperties(_PhysicalDevice))
	, _PhysicalDeviceFeatures(GetPhysicalDeviceFeatures(_PhysicalDevice))
	, _VulkanCache(*this)
{
	/** Create the logical device. */
	const std::unordered_set<int32> uniqueQueueFamilies = _Queues.GetUniqueFamilies();
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
		.enabledExtensionCount = static_cast<uint32>(gDeviceExtensions.size()),
		.ppEnabledExtensionNames = gDeviceExtensions.data(),
	};
	
	if (deviceDesc.enableValidationLayers)
	{
		deviceInfo.enabledLayerCount = static_cast<uint32>(gValidationLayers.size());
		deviceInfo.ppEnabledLayerNames = gValidationLayers.data();
	}
	else
	{
		deviceInfo.enabledLayerCount = 0;
	}

	vulkan(vkCreateDevice(_PhysicalDevice, &deviceInfo, nullptr, &_Device));

	_Queues.Create(_Device);
	
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