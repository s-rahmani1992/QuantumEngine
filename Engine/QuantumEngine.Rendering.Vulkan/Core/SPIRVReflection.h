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

	struct DescriptableBufferData {
		std::string name;
		UInt32 offsetIndex;
		VkDescriptorType descriptorType;
		SpvReflectDescriptorBinding data;
	};

	class SPIRVReflection
	{
	public:
		SPIRVReflection() = default;
		void AddShaderReflection(const SpvReflectShaderModule* shaderReflection);
		UInt32 GetDescriptorLayoutCount();
		UInt32 GetDynamicDescriptorCount();
		void CreatePipelineLayout(const VkDevice device, const VkSampler sampler, VkPipelineLayout* pipelineLayout, VkDescriptorSetLayout* m_descriptorSetLayout);
		MaterialReflection CreateMaterialReflection();
		inline std::vector<PushConstantBufferData>& GetPushConstants() { return m_pushConstants; }
		inline std::vector<DescriptableBufferData>& GetDescriptors() { return m_descripters; }
		DescriptableBufferData* GetDescriptorData(const std::string name);
		void Initializes();
	private:
		std::vector<PushConstantBufferData> m_pushConstants;
		std::vector<DescriptableBufferData> m_descripters;
		std::vector<DescriptableBufferData> m_samplers;
	};
}