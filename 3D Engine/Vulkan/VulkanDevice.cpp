#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanDRM.h"
#include <Platform/WindowsPlatform.h> // Yuck
#include <GLFW/glfw3.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT Flags, VkDebugReportObjectTypeEXT ObjType,
	uint64 Obj, size_t Location, int32 Code, const char* LayerPrefix, const char* Msg, void* UserData)
{
	print("Validation layer: %s\nLayer prefix: %s", Msg, LayerPrefix);
	return VK_FALSE;
}

static std::vector<const char*> GetRequiredExtensions()
{
	uint32 GLFWExtensionCount = 0;
	const char** GLFWExtensions;
	GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);
	std::vector<const char*> Extensions(GLFWExtensions, GLFWExtensions + GLFWExtensionCount);

#ifdef DEBUG_BUILD
	Extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	return Extensions;
}

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

static bool CheckDeviceExtensionSupport(VkPhysicalDevice Device, const std::vector<const char*> &DeviceExtensions)
{
	uint32 ExtensionCount;
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);

	std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data());

	std::unordered_set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

	for (const auto &Extension : AvailableExtensions)
	{
		RequiredExtensions.erase(Extension.extensionName);
	}

	return RequiredExtensions.empty();
}

static bool IsDeviceSuitable(VkPhysicalDevice Device, VkSurfaceKHR Surface, const std::vector<const char*>& DeviceExtensions)
{
	QueueFamilyIndices Indices = {};
	Indices.FindQueueFamilies(Device, Surface);

	bool ExtensionsSupported = CheckDeviceExtensionSupport(Device, DeviceExtensions);
	bool SwapChainAdequate = false;

	if (ExtensionsSupported)
	{
		SwapchainSupportDetails SwapchainSupport = {};
		SwapchainSupport.QuerySwapchainSupport(Device, Surface);
		SwapChainAdequate = !SwapchainSupport.Formats.empty() && !SwapchainSupport.PresentModes.empty();
	}

	VkPhysicalDeviceFeatures SupportedFeatures;
	vkGetPhysicalDeviceFeatures(Device, &SupportedFeatures);

	return Indices.IsComplete() && ExtensionsSupported && SwapChainAdequate && SupportedFeatures.samplerAnisotropy;
}

static VkPhysicalDevice SelectPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, const std::vector<const char*>& DeviceExtensions)
{
	uint32 DeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

	if (DeviceCount == 0)
	{
		fail("Failed to find GPUs with Vulkan support.");
	}

	std::vector<VkPhysicalDevice> Devices(DeviceCount);
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

	for (const auto &Device : Devices)
	{
		if (IsDeviceSuitable(Device, Surface, DeviceExtensions))
		{
			PhysicalDevice = Device;
			break;
		}
	}

	if (PhysicalDevice == VK_NULL_HANDLE)
	{
		fail("Failed to find a suitable GPU.");
	}

	return PhysicalDevice;
}

static VkDevice CreateLogicalDevice(VkPhysicalDevice PhysicalDevice,
	VkSurfaceKHR Surface,
	const std::vector<const char*>& DeviceExtensions,
	const std::vector<const char*>& ValidationLayers,
	VkQueue& GraphicsQueue,
	VkQueue& PresentQueue)
{
	QueueFamilyIndices Indices = {};
	Indices.FindQueueFamilies(PhysicalDevice, Surface);

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::unordered_set<int> UniqueQueueFamilies = { Indices.GraphicsFamily, Indices.PresentFamily };

	float QueuePriority = 1.0f;
	for (int QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		QueueCreateInfo.queueFamilyIndex = QueueFamily;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};
	DeviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	CreateInfo.queueCreateInfoCount = static_cast<uint32>(QueueCreateInfos.size());
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	CreateInfo.pEnabledFeatures = &DeviceFeatures;
	CreateInfo.enabledExtensionCount = static_cast<uint32>(DeviceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

#ifdef DEBUG_BUILD
	CreateInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
	CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
#else
	CreateInfo.enabledLayerCount = 0;
#endif

	VkDevice Device;
	vulkan(vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device));

	vkGetDeviceQueue(Device, Indices.GraphicsFamily, 0, &GraphicsQueue);
	vkGetDeviceQueue(Device, Indices.PresentFamily, 0, &PresentQueue);

	return Device;
}

static VkCommandPool CreateCommandPool(VkDevice Device,
	VkPhysicalDevice PhysicalDevice,
	VkSurfaceKHR Surface)
{
	QueueFamilyIndices QueueFamilyIndices = {};
	QueueFamilyIndices.FindQueueFamilies(PhysicalDevice, Surface);

	VkCommandPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	PoolInfo.queueFamilyIndex = QueueFamilyIndices.GraphicsFamily;
	PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool CommandPool;
	vulkan(vkCreateCommandPool(Device, &PoolInfo, nullptr, &CommandPool));

	return CommandPool;
}

VulkanDevice::VulkanDevice()
{
	const std::vector<const char*> ValidationLayers =
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	const std::vector<const char*> DeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
	};

#ifdef DEBUG_BUILD
	if (!CheckValidationLayerSupport(ValidationLayers))
	{
		fail("Validation layers requested, but are unavailable.");
	}
#endif

	// Create instance
	VkApplicationInfo ApplicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	ApplicationInfo.pApplicationName = "Vulkan Engine";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pEngineName = "No Engine";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	auto Extensions = GetRequiredExtensions();

	VkInstanceCreateInfo InstanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	InstanceInfo.pApplicationInfo = &ApplicationInfo;
	InstanceInfo.enabledExtensionCount = static_cast<uint32>(Extensions.size());
	InstanceInfo.ppEnabledExtensionNames = Extensions.data();

#ifdef DEBUG_BUILD
	InstanceInfo.enabledLayerCount = static_cast<uint32>(ValidationLayers.size());
	InstanceInfo.ppEnabledLayerNames = ValidationLayers.data();
#else
	InstanceInfo.enabledLayerCount = 0;
#endif

	vulkan(vkCreateInstance(&InstanceInfo, nullptr, &Instance));

#ifdef DEBUG_BUILD
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

	vulkan(CreateDebugReportCallbackEXT(Instance, &DebugCallbackInfo, nullptr, &DebugReportCallback));
#endif

#ifdef _WIN32
	WindowsPlatformRef Windows = std::static_pointer_cast<WindowsPlatform>(GPlatform);
	vulkan(glfwCreateWindowSurface(Instance, Windows->Window, nullptr, &Surface));
#elif
	signal_unimplemented();
#endif

	// Select Vulkan-capable physical device
	PhysicalDevice = SelectPhysicalDevice(Instance, Surface, DeviceExtensions);

	// Create logical device 
	Device = CreateLogicalDevice(PhysicalDevice, Surface, DeviceExtensions, ValidationLayers, GraphicsQueue, PresentQueue);

	// Create command pool
	CommandPool = CreateCommandPool(Device, PhysicalDevice, Surface);

	// Get physical device properties/features
	vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);
}

VulkanDevice::~VulkanDevice()
{
	for (const auto&[GraphicsPipelineState, PipelineLayout, DescriptorSetLayout] : PipelineLayoutCache)
	{
		vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	}

	for (const auto&[PSOInit, Pipeline] : PipelineCache)
	{
		vkDestroyPipeline(Device, Pipeline, nullptr);
	}

	for (const auto&[RPInit, RenderPass, Framebuffer] : RenderPassCache)
	{
		vkDestroyFramebuffer(Device, Framebuffer, nullptr);
		vkDestroyRenderPass(Device, RenderPass, nullptr);
	}
}

inline bool QueueFamilyIndices::IsComplete() const
{
	return GraphicsFamily >= 0 && PresentFamily >= 0;
}

inline void QueueFamilyIndices::FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface)
{
	QueueFamilyIndices Indices;

	uint32 QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

	for (int32 i = 0; i < static_cast<int32>(QueueFamilies.size()); i++)
	{
		if (QueueFamilies[i].queueCount > 0 && QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			GraphicsFamily = i;
		}

		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &PresentSupport);

		if (QueueFamilies[i].queueCount > 0 && PresentSupport)
		{
			PresentFamily = i;
		}

		if (Indices.IsComplete())
		{
			break;
		}
	}
}