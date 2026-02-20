#pragma once
#include "vulkan-pch.h"

namespace QuantumEngine::Rendering {
	class Material;
}

namespace QuantumEngine::Rendering::Vulkan::Rasterization {
	class SPIRVRasterizationProgram;

	class VulkanRasterizationMaterial {
	public:
		VulkanRasterizationMaterial(const ref<Material>& material);
		void BindValues(VkCommandBuffer commandBuffer);

	private:
		struct pushConstantData {
			UInt32 offset;
			Byte* location;
			UInt32 size;
		};

		ref<Material> m_material;
		ref<SPIRVRasterizationProgram> m_program;
		VkPipelineLayout m_pipelineLayout;
		std::vector<pushConstantData> m_pushConstantValues;
	};
}