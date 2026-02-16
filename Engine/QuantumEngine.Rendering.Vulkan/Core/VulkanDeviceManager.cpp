#include "vulkan-pch.h"
#include "VulkanDeviceManager.h"
#include "Platform/Application.h"
#include "Platform/GraphicWindow.h"
#include "VulkanGraphicContext.h"
#include "VulkanShaderRegistery.h"
#include <set>

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

	enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	enabledExtensions.push_back("VK_KHR_win32_surface");

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
	auto tempWindow = Platform::Application::CreateGraphicWindow(Platform::WindowProperties{ 100, 100, L"Temp Window" });
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0) // No Vulkan compatible devices found
		return false;

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
	VkPhysicalDeviceProperties deviceProperties;

	VkSurfaceKHR tempSurface;
	VkWin32SurfaceCreateInfoKHR createInfo
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = tempWindow->GetHandle(),
	};

	result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &tempSurface);

	if (result != VK_SUCCESS)
		return false;

	bool deviceFound = false;
	Int32 surfaceFamilyIndex = -1;
	std::vector<const char*> requiredDeviceExtensions;
	requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	for (auto& devicePtr : devices) {
		vkGetPhysicalDeviceProperties(devicePtr, &deviceProperties);
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			CheckQueueSupport(devicePtr) &&
			CheckDeviceExtensionSupport(devicePtr, requiredDeviceExtensions) &&
			CheckDeviceSwapChainSupport(devicePtr, tempSurface))
		{
			surfaceFamilyIndex = FindPresentFamilies(devicePtr, tempSurface);
			if(surfaceFamilyIndex == -1)
				continue;

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

	std::set<UInt32> uniqueQueueFamilies = { (UInt32)graphicsQueueFamilyIndex, (UInt32)surfaceFamilyIndex };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	for(auto& queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		};
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = (UInt32)queueCreateInfos.size(),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = (UInt32)requiredDeviceExtensions.size(),
		.ppEnabledExtensionNames = requiredDeviceExtensions.data(),
		.pEnabledFeatures = &deviceFeatures,
	};

	result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_graphicDevice);

	if (result != VK_SUCCESS)
		return false;

	m_graphicsQueueFamilyIndex = (UInt32)graphicsQueueFamilyIndex;
	m_surfaceQueueFamilyIndex = (UInt32)surfaceFamilyIndex;

	vkDestroySurfaceKHR(m_instance, tempSurface, nullptr);
	DestroyWindow(tempWindow->GetHandle());
	return true;
}

ref<QuantumEngine::Rendering::GraphicContext> QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CreateHybridContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window)
{
	ref<VulkanGraphicContext> context = std::make_shared<VulkanGraphicContext>(m_instance, m_physicalDevice, m_graphicDevice, m_graphicsQueueFamilyIndex, m_surfaceQueueFamilyIndex, window);

	if(context->Initialize() == false)
		return nullptr;

	return context;
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
	ref<VulkanShaderRegistery> shaderRegistery = std::make_shared<VulkanShaderRegistery>(m_graphicDevice);
	return shaderRegistery;
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
#if defined(_DEBUG)

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

#endif

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

Int32 QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::FindPresentFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	UInt32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	VkBool32 presentSupport = false;

	for (int i = 0; i < queueFamilyCount; i++) {
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport)
			return i;
	}

	return -1;
}

bool QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> tempExtensions(requiredExtensions.begin(), requiredExtensions.end());

	for (const auto& extension : availableExtensions) {
		tempExtensions.erase(extension.extensionName);
	}

	return tempExtensions.empty();
}

bool QuantumEngine::Rendering::Vulkan::VulkanDeviceManager::CheckDeviceSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	return formatCount > 0 && presentModeCount > 0;
}
