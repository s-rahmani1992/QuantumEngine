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
		bool Initialize(const VkPhysicalDeviceMemoryProperties* memoryProperties);
		inline VkBuffer GetVertexBuffer() { return m_vertexBuffer; }
		inline VkBuffer GetIndexBuffer() { return m_indexBuffer; }
		void CopyCommand(VkCommandBuffer commandBuffer, VkBuffer stageBuffer, UInt32 offset);
	
	private:

		ref<Mesh> m_mesh;
		VkDevice m_device;

		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;

		VkBuffer m_indexBuffer;
		VkDeviceMemory m_indexBufferMemory;
	};
}
