#pragma once

#include "vulkan-pch.h"
#include "Rendering/ShaderProgram.h"
#include "../Core/SPIRVReflection.h"

namespace QuantumEngine::Rendering::Vulkan {
	class SPIRVShader;
}

namespace QuantumEngine::Rendering::Vulkan::Rasterization {
	class SPIRVRasterizationProgram : public ShaderProgram
	{
	public:
		SPIRVRasterizationProgram(const std::vector<ref<SPIRVShader>>& spirvShaders, const VkDevice device);
		virtual ~SPIRVRasterizationProgram() override;
		inline std::vector<VkPipelineShaderStageCreateInfo>& GetStageInfos() { return m_stageInfos; }
		inline VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		inline SPIRVReflection& GetReflection() { return m_reflection; }
		inline std::vector<VkDescriptorSetLayout>& GetDiscriptorLayouts() { return m_descriptorSetLayout; }
	private:
		void InitializeSampler();
		ref<SPIRVShader> m_vertexShader;
		ref<SPIRVShader> m_geometryShader;
		ref<SPIRVShader> m_pixelShader;

		const VkDevice m_device;
		std::vector<VkPipelineShaderStageCreateInfo> m_stageInfos;
		VkPipelineLayout m_pipelineLayout;
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayout;
		SPIRVReflection m_reflection;
		VkSampler m_sampler;
	};
}