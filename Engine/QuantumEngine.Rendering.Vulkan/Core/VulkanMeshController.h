#pragma once
#include "vulkan-pch.h"
#include "Rendering/GPUMeshController.h"

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanMeshController : public GPUMeshController
	{
	public: 		
		VulkanMeshController(const ref<Mesh>& mesh, const VkDevice device);
		~VulkanMeshController() override;
		bool Initialize(VkPhysicalDevice physicalDevice);
	private:
		UInt32 GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags properties, const VkPhysicalDeviceMemoryProperties* memoryProperties);

		ref<Mesh> m_mesh;
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;

		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;

		VkBuffer m_indexBuffer;
		VkDeviceMemory m_indexBufferMemory;
	};
}
