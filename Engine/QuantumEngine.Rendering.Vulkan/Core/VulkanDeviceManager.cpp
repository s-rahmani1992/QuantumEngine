#include "vulkan-pch.h"
#include "VulkanDeviceManager.h"
#include "Platform/GraphicWindow.h"

bool QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::Initialize()
{
	// Get Supported Extensions
	UInt32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Get Supported Layers
	UInt32 layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> layers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	// Creating Vulkan Instance
	VkApplicationInfo appInfo{ 
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
		.pNext = nullptr, 
		.pApplicationName = "Vulkan Engine", 
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0), 
		.pEngineName = "No Engine", 
		.engineVersion = VK_MAKE_VERSION(1, 0, 0), 
		.apiVersion = VK_API_VERSION_1_3, };

	std::vector<const char*> enabledLayers;
	std::vector<const char*> enabledExtensions;

#if defined(_DEBUG)

	enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
	enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#endif

	VkInstanceCreateInfo vkCreateInfo
	{ 
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 
		.pNext = nullptr, 
		.flags = 0, 
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = (UInt32)enabledLayers.size(),
		.ppEnabledLayerNames = enabledLayers.size() == 0 ? nullptr : enabledLayers.data(),
		.enabledExtensionCount = (UInt32)enabledExtensions.size(),
		.ppEnabledExtensionNames = enabledExtensions.size() == 0 ? nullptr : enabledExtensions.data(),
	};

	VkResult result = vkCreateInstance(&vkCreateInfo, nullptr, &m_instance);

	if(result != VK_SUCCESS)
		return false;

#if defined(_DEBUG)

	// Setup Debug Messenger
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = DebugMessageCallback,
		.pUserData = nullptr,
	};

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");

	func(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger);

#endif

	// Getting suitable physical device (preferably discrete GPU)
	UInt32 deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0) // No Vulkan compatible devices found
		return false;

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
	VkPhysicalDeviceProperties deviceProperties;

	bool deviceFound = false;

	for(auto& devicePtr : devices) {
		vkGetPhysicalDeviceProperties(devicePtr, &deviceProperties);
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			CheckQueueSupport(devicePtr)) 
		{
			deviceFound = true;
			m_physicalDevice = devicePtr;
			break;
		}
	}

	if (deviceFound == false) // No Suitable discrete GPU found
		return false;


	// Create Logical Device and Graphics Queue
	float queuePriority = 1.0f;
	Int32 graphicsQueueFamilyIndex = FindQueueFamilies(m_physicalDevice, VK_QUEUE_GRAPHICS_BIT);

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceQueueCreateInfo deviceQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = (UInt32)graphicsQueueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};

	VkDeviceCreateInfo deviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &deviceQueueCreateInfo,
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = nullptr,
		.pEnabledFeatures = &deviceFeatures,
	};

	result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_graphicDevice);

	if (result != VK_SUCCESS)
		return false;

	vkGetDeviceQueue(m_graphicDevice, (UInt32)graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

	return true;
}

ref<QuantumEngine::Rendering::GraphicContext> QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CreateHybridContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window)
{
	return nullptr;
}

ref<QuantumEngine::Rendering::GraphicContext> QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CreateRayTracingContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window)
{
	return nullptr;
}

ref<QuantumEngine::Rendering::GPUAssetManager> QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CreateAssetManager()
{
	return nullptr;
}

ref<QuantumEngine::Rendering::ShaderRegistery> QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CreateShaderRegistery()
{
	return nullptr;
}

ref<QuantumEngine::Rendering::MaterialFactory> QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CreateMaterialFactory()
{
	return nullptr;
}

QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::~VulkanDeviceManager()
{
#if defined(_DEBUG)
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
	func(m_instance, m_debugMessenger, nullptr);
#endif

	vkDestroyDevice(m_graphicDevice, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {// consider warning and error messages only
		std::string message = "[VULKAN ";
		message += (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ? "ERROR] --> " : "WARNING] --> ";
		message += pCallbackData->pMessage;
		OutputDebugStringA(message.c_str());
	}
	
	return VK_FALSE;
}

bool QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CheckQueueSupport(VkPhysicalDevice device)
{
	UInt32 targetFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
	UInt32 flags = 0;

	UInt32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for(auto & queueFamily : queueFamilies) {
		flags |= (queueFamily.queueFlags & targetFlags);
	}

	return flags == targetFlags;
}

Int32 QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::FindQueueFamilies(VkPhysicalDevice device, UInt32 flag)
{
	UInt32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for(int i = 0; i < queueFamilyCount; i++) {
		if ((queueFamilies[i].queueFlags & flag) == flag)
			return i;
	}

	return -1;
}
