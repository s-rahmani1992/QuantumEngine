#pragma once

#include "vulkan-pch.h"
#include "Rendering/GPUDeviceManager.h"
#include "Rendering/GraphicContext.h"
#include "Rendering/GPUAssetManager.h" 
#include "Rendering/ShaderRegistery.h" 
#include "Rendering/MaterialFactory.h"

namespace QuantumEngine {
	namespace Platform {
		class GraphicWindow;
	}
}

namespace QuantumEngine::Rendering::Vulkan {

	class VulkanDeviceManager : public GPUDeviceManager
	{
	public:
		virtual bool Initialize() override;
		virtual ref<GraphicContext> CreateHybridContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window) override;
		virtual ref<GraphicContext> CreateRayTracingContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window) override;
		virtual ref<GPUAssetManager> CreateAssetManager() override;
		virtual ref<ShaderRegistery> CreateShaderRegistery() override;
		virtual ref<MaterialFactory> CreateMaterialFactory() override;
		~VulkanDeviceManager();

	private:

#if defined(_DEBUG)
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif

		bool CheckQueueSupport(VkPhysicalDevice device);
		Int32 FindQueueFamilies(VkPhysicalDevice device, UInt32 flag);

		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_graphicDevice;
		VkQueue m_graphicsQueue;
	};
}