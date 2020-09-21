#include "VulkanDevice.h"

#if _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <unordered_set>

static bool CheckValidationLayerSupport(const std::vector<const char*>& ValidationLayers)
{
	uint32 LayerCount;
	vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
	std::vector<VkLayerProperties> AvailableLayers(LayerCount);
	vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

	for (const char* LayerName : ValidationLayers)
	{
		if (![] (const char* LayerName, const std::vector<VkLayerProperties>& AvailableLayers)
		{
			for (const auto& LayerProperties : AvailableLayers)
			{
				if (strcmp(LayerName, LayerProperties.layerName) == 0)
				{
					return true;
				}
			}

			return false;
		} (LayerName, AvailableLayers))
		{
			return false;
		}
	}

	return true;
}

static VkInstance CreateInstance(const std::vector<const char*>& ValidationLayers, bool bUseValidationLayers)
{
	if (bUseValidationLayers && !CheckValidationLayerSupport(ValidationLayers))
	{
		fail("Validation layers requested, but are unavailable.");
	}

	VkApplicationInfo ApplicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	ApplicationInfo.pApplicationName = "Vulkan Engine";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pEngineName = "No Engine";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.apiVersion = VK_API_VERSION_1_2;

	std::vector<const char*> Extensions;

	if (bUseValidationLayers)
	{
		Extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	Extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	Extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if _WIN32
	Extensions.push_back("VK_KHR_win32_surface");
#endif

	VkInstanceCreateInfo InstanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	InstanceInfo.pApplicationInfo = &ApplicationInfo;
	InstanceInfo.enabledExtensionCount = static_cast<uint32>(Extensions.size());
	InstanceInfo.ppEnabledExtensionNames = Extensions.data();

	if (bUseValidationLayers)
	{
		InstanceInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
		InstanceInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else
	{
		InstanceInfo.enabledLayerCount = 0;
	}

	VkInstance Instance;
	vulkan(vkCreateInstance(&InstanceInfo, nullptr, &Instance));

	return Instance;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT Flags, VkDebugReportObjectTypeEXT ObjType,
	uint64 Obj, size_t Location, int32 Code, const char* LayerPrefix, const char* Msg, void* UserData)
{
	LOG("Validation layer: %s\nLayer prefix: %s", Msg, LayerPrefix);
	return VK_FALSE;
}

static VkDebugReportCallbackEXT CreateDebugReportCallback(VkInstance Instance, bool bUseValidationLayers)
{
	if (bUseValidationLayers)
	{
		// Create Vulkan debug callback
		VkDebugReportCallbackCreateInfoEXT DebugCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		DebugCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		DebugCallbackInfo.pfnCallback = DebugCallback;

		auto CreateDebugReportCallbackEXT = [] (VkInstance Instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
		{
			auto Func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugReportCallbackEXT");
			if (Func)
			{
				return Func(Instance, pCreateInfo, pAllocator, pCallback);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		};

		VkDebugReportCallbackEXT DebugReportCallback;
		vulkan(CreateDebugReportCallbackEXT(Instance, &DebugCallbackInfo, nullptr, &DebugReportCallback));
	}

	return 0;
}

static bool AllDeviceExtensionsSupported(VkPhysicalDevice Device, const std::vector<const char*>& DeviceExtensions)
{
	uint32 ExtensionCount;
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);

	std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data());

	std::unordered_set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

	for (const auto& Extension : AvailableExtensions)
	{
		RequiredExtensions.erase(Extension.extensionName);
	}

	return RequiredExtensions.empty();
}

/** Select a Vulkan-capable physical device. */
static VkPhysicalDevice SelectPhysicalDevice(VkInstance Instance, const std::vector<const char*>& DeviceExtensions)
{
	uint32 DeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

	check(DeviceCount != 0, "Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> Devices(DeviceCount);
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

	for (const auto& Device : Devices)
	{
		if (AllDeviceExtensionsSupported(Device, DeviceExtensions))
		{
			PhysicalDevice = Device;
			break;
		}
	}

	check(PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU.");

	return PhysicalDevice;
}

static VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice PhysicalDevice)
{
	VkPhysicalDeviceProperties Properties;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
	return Properties;
}

static VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice PhysicalDevice)
{
	VkPhysicalDeviceFeatures Features;
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);
	return Features;
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
	: Instance(CreateInstance(ValidationLayers, deviceDesc.enableValidationLayers))
	, DebugReportCallback(CreateDebugReportCallback(Instance, deviceDesc.enableValidationLayers))
	, PhysicalDevice(SelectPhysicalDevice(Instance, DeviceExtensions))
	, _Surface(CreateSurface(deviceDesc.windowHandle, Instance))
	, Queues(*this)
	, Properties(GetPhysicalDeviceProperties(PhysicalDevice))
	, Features(GetPhysicalDeviceFeatures(PhysicalDevice))
	, VulkanCache(*this)
{
	/** Create the logical device. */
	const std::unordered_set<int32> UniqueQueueFamilies = Queues.GetUniqueFamilies();
	const float QueuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;

	for (int32 QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		QueueCreateInfo.queueFamilyIndex = QueueFamily;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures Features = {};
	Features.samplerAnisotropy = VK_TRUE;
	Features.geometryShader = VK_TRUE;
	Features.fragmentStoresAndAtomics = VK_TRUE;
	Features.vertexPipelineStoresAndAtomics = VK_TRUE;
	Features.shaderStorageImageWriteWithoutFormat = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };
	DescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	DescriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
	DescriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	DescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
	DescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

	VkPhysicalDeviceFeatures2 Features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	Features2.pNext = &DescriptorIndexingFeatures;
	Features2.features = Features;

	VkDeviceCreateInfo DeviceInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	DeviceInfo.pNext = &Features2;
	DeviceInfo.queueCreateInfoCount = static_cast<uint32>(QueueCreateInfos.size());
	DeviceInfo.pQueueCreateInfos = QueueCreateInfos.data();
	DeviceInfo.enabledExtensionCount = static_cast<uint32>(DeviceExtensions.size());
	DeviceInfo.ppEnabledExtensionNames = DeviceExtensions.data();

	if (deviceDesc.enableValidationLayers)
	{
		DeviceInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
		DeviceInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else
	{
		DeviceInfo.enabledLayerCount = 0;
	}

	vulkan(vkCreateDevice(PhysicalDevice, &DeviceInfo, nullptr, &Device));

	Queues.Create(Device);
	
	const VmaAllocatorCreateInfo allocatorInfo =
	{
		.physicalDevice = PhysicalDevice,
		.device = Device,
		.instance = Instance
	};

	vulkan(vmaCreateAllocator(&allocatorInfo, &_Allocator));
	
	BindlessTextures = std::make_shared<gpu::BindlessDescriptors>(Device, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 65556);
	gBindlessTextures = BindlessTextures;

	BindlessSamplers = std::make_shared<gpu::BindlessDescriptors>(Device, VK_DESCRIPTOR_TYPE_SAMPLER, 1024);
	gBindlessSamplers = BindlessSamplers;

	BindlessImages = std::make_shared<gpu::BindlessDescriptors>(Device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256);
	gBindlessImages = BindlessImages;
}