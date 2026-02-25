#pragma once

#include "vulkan-pch.h"
#include "Rendering/GraphicContext.h"

namespace QuantumEngine {
	namespace Platform {
		class GraphicWindow;
	}
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanBufferFactory;

	namespace Rasterization {
		class VulkanRasterizationPipelineModule;
	}

	struct TransformGPU {
	public:
		Matrix4 modelMatrix;
		Matrix4 rotationMatrix;
		Matrix4 modelViewMatrix;
	};

	struct VKEntityGPUData {
	public:
		ref<GameEntity> gameEntity;
		UInt32 index;
	};

	class VulkanGraphicContext : public GraphicContext
	{
	public:
		VulkanGraphicContext(const VkInstance vkInstance, VkPhysicalDevice physicalDevice, VkDevice logicDevice, UInt32 graphicsQueueFamilyIndex, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window);
		~VulkanGraphicContext();
		bool Initialize(const ref<VulkanBufferFactory> bufferFactory);
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& assetManager) override;
		virtual void RegisterShaderRegistery(const ref<ShaderRegistery>& shaderRegistery) override;
		virtual bool PrepareScene(const ref<Scene>& scene) override;
		virtual void Render() override;

	private:
		void InitializeLightBuffer(const SceneLightData& lightData);
		void UpdateTransforms();

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

		std::vector<ref<Rasterization::VulkanRasterizationPipelineModule>> m_rasterizationModules;
		VkPhysicalDeviceMemoryProperties m_memoryProperties;
		std::vector<VKEntityGPUData> m_entityGPUList;
		UInt32 m_transformStride;
		VkBuffer m_transformBuffer;
		VkDeviceMemory m_transformBufferMemory;
		TransformGPU m_transformData;
		ref<VulkanBufferFactory> m_bufferFactory;
		VkDescriptorPool m_descriptorPool;

		ref<Camera> m_camera;
		VkBuffer m_cameraBuffer;
		VkDeviceMemory m_cameraBufferMemory;
		UInt32 m_cameraStride;
		CameraGPU m_cameraGPU;

		VkBuffer m_lightBuffer;
		VkDeviceMemory m_lightBufferMemory;
		UInt32 m_lightStride;
	};
}
