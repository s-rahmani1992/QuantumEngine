#pragma once
#include "vulkan-pch.h"
#include <Rendering/Material.h>

namespace QuantumEngine::Rendering::Vulkan {
	struct PushConstantVariableData {
		std::string name;
		SpvReflectBlockVariable variableDesc;
	};

	struct PushConstantBufferData { //TODO Add shader stage flags to make it more optimal for binding
		std::string name;
		SpvReflectBlockVariable data;
		std::vector<PushConstantVariableData> variables;
	};

	class SPIRVReflection
	{
	public:
		SPIRVReflection() = default;
		void AddShaderReflection(const SpvReflectShaderModule* shaderReflection);
		VkPipelineLayout CreatePipelineLayout(const VkDevice device);
		MaterialReflection CreateMaterialReflection();
		inline std::vector<PushConstantBufferData>& GetPushConstants() { return m_pushConstants; }
	private:
		std::vector<PushConstantBufferData> m_pushConstants;
	};
}