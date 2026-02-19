#pragma once
#include "vulkan-pch.h"
#include "Rendering/GPUAssetManager.h"

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanAssetManager : public GPUAssetManager 
	{
	public:
		VulkanAssetManager(const VkDevice device, VkPhysicalDevice physicalDevice);
		~VulkanAssetManager();
		bool Initializes(UInt32 familyIndex);
		virtual void UploadMeshToGPU(const ref<Mesh>& mesh) override;
		virtual void UploadTextureToGPU(const ref<Texture2D>& texture) override;
		virtual void UploadMeshesToGPU(const std::vector<ref<Mesh>>& meshes) override;

	private:
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		VkCommandPool m_commandPool;
		VkCommandBuffer m_commandBuffer;
		VkQueue m_graphicsQueue;
		VkPhysicalDeviceMemoryProperties m_memoryProperties;
	};
}