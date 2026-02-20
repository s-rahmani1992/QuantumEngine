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
	private:
		ref<SPIRVShader> m_vertexShader;
		ref<SPIRVShader> m_geometryShader;
		ref<SPIRVShader> m_pixelShader;

		const VkDevice m_device;
		std::vector<VkPipelineShaderStageCreateInfo> m_stageInfos;
		VkPipelineLayout m_pipelineLayout;
		SPIRVReflection m_reflection;
	};
}