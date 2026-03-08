#pragma once
#include "vulkan-pch.h"
#include <map>

namespace QuantumEngine::Rendering::Vulkan {
	struct VKEntityGPUData;
	class VulkanBufferFactory;
	class VulkanMeshController;
}

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class VulkanBLAS;

	class VulkanRayTracingPipelineModule {
	public:
		VulkanRayTracingPipelineModule();
		~VulkanRayTracingPipelineModule();

		bool Initialize(std::vector<VKEntityGPUData>& entities);

	private:
		VkDevice m_device;
		ref<VulkanBufferFactory> m_bufferFactory;

		std::map<ref<VulkanMeshController>, ref<VulkanBLAS>> m_blasMap;
		VkQueue m_graphicsQueue;
	};
}