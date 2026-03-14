#pragma once
#include "vulkan-pch.h"
#include <map>

namespace QuantumEngine {
	class GameEntity;
}

namespace QuantumEngine::Rendering {
	class Material;
}

namespace QuantumEngine::Rendering::Vulkan {
	struct VKEntityGPUData;
	class VulkanBufferFactory;
	class VulkanMeshController;
}

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class VulkanBLAS;
	class SPIRVRayTracingProgram;

	class VulkanRayTracingPipelineModule {
	public:
		VulkanRayTracingPipelineModule();
		~VulkanRayTracingPipelineModule();

		bool Initialize(std::vector<ref<GameEntity>>& entities, const ref<Material> rtMaterial, VkBuffer camBuffer, VkBuffer lightBuffer, UInt32 camStride, UInt32 lightStride, const VkExtent2D& extent);
		void RenderCommand(VkCommandBuffer commandBuffer);
		VkImage GetOutputImage() const { return m_outputImage; }
	private:
		void CreateOutputImage();
		void CreatePipelineAndSBT();
		void WriteBuffers(const std::string name, const VkBuffer buffer, UInt32 stride);

		struct VKEntityGPUData {
		public:
			ref<GameEntity> gameEntity;
			UInt32 index;
		};

		struct pushConstantData {
			UInt32 offset;
			Byte* location;
			UInt32 size;
		};

		struct DescriptorData {
			UInt32 setIndex;
			UInt32 binding;
			VkDescriptorType descriptorType;
		};

		VkDevice m_device;
		VkDescriptorPool m_descriptorPool;
		ref<VulkanBufferFactory> m_bufferFactory;

		std::map<ref<VulkanMeshController>, ref<VulkanBLAS>> m_blasMap;
		VkQueue m_graphicsQueue;
		VkExtent2D m_extent;
		VkAccelerationStructureKHR m_tlas;
		VkDeviceAddress m_tlasAddress;
		VkBuffer m_tlasBuffer;
		VkDeviceMemory m_tlasMemory;

		VkBuffer m_SBT;
		VkDeviceMemory m_SBTMemory;

		ref<SPIRVRayTracingProgram> m_globalRtProgram;
		VkPipeline m_rtPipeline;

		std::vector<pushConstantData> m_pushConstantValues;
		std::vector<VkDescriptorSet> m_descriptorSets;		
		std::vector<DescriptorData> m_descriptorData;

		VkImage m_outputImage;
		VkDeviceMemory m_outputImageMemory;
		VkImageView m_outputImageView;

		VkStridedDeviceAddressRegionKHR m_rayGenRegion;
		VkStridedDeviceAddressRegionKHR m_missRegion;
		VkStridedDeviceAddressRegionKHR m_hitRegion;
		VkStridedDeviceAddressRegionKHR m_callableRegion = {0, 0, 0};
	};
}