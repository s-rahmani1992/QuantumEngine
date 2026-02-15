#pragma once

#include "vulkan-pch.h"
#include "Rendering/GraphicContext.h"

namespace QuantumEngine {
	namespace Platform {
		class GraphicWindow;
	}
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanGraphicContext : public GraphicContext
	{
	public:
		VulkanGraphicContext(const VkInstance vkInstance, VkPhysicalDevice physicalDevice, VkDevice logicDevice, UInt32 graphicsQueueFamilyIndex, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window);
		~VulkanGraphicContext();
		bool Initialize();
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& assetManager) override;
		virtual void RegisterShaderRegistery(const ref<ShaderRegistery>& shaderRegistery) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		ref<QuantumEngine::Platform::GraphicWindow> m_window;		
		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_logicDevice;

		VkSurfaceKHR m_surface;
		VkSurfaceFormatKHR m_swapChainFormat;
		VkSwapchainKHR m_swapChain;
		VkSurfaceCapabilitiesKHR m_swapChainCapability;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;

		VkRenderPass m_renderPass;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		UInt32 m_graphicsQueueFamilyIndex;

		VkCommandPool m_commandPool;
		VkCommandBuffer m_commandBuffer;

		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;

		VkSemaphore m_imageAvailableSemaphore;
		VkSemaphore m_renderFinishedSemaphore;
		VkFence m_fence;
	};
}
