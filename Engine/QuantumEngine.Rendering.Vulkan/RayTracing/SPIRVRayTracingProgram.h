#pragma once
#include "vulkan-pch.h"
#include "Core/SPIRVShaderProgram.h"

namespace QuantumEngine::Rendering::Vulkan::RayTracing {
	class SPIRVRayTracingProgramVariant;

	struct ShaderRecordVariableData {
		std::string name;
		UInt32 offset;
		UInt32 size;
	};

	class SPIRVRayTracingProgram : public SPIRVShaderProgram {
	public:
		SPIRVRayTracingProgram(Byte* bytecode, UInt64 codeSize, VkDevice device);
		~SPIRVRayTracingProgram();
		std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages();
		//inline VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		//inline std::vector<VkDescriptorSetLayout>& GetDiscriptorLayouts() { return m_descriptorSetLayout; }
		ref<SPIRVRayTracingProgramVariant> CreateVariantForRT(UInt32& startBinding);
		UInt32 GetShaderRecordSize();
		inline bool HasMissStage() { return m_missEntryPoint != nullptr; }
		bool HasHitGroup();
		std::vector<ShaderRecordVariableData>& GetShaderRecordVariables() { return m_shaderRecordVariables; }
	private:

		Byte* m_bytecode;
		UInt64 m_codeSize;
		VkShaderModule m_rtModule;
		SpvReflectShaderModule m_reflectionModule;

		SpvReflectEntryPoint* m_rayGenEntryPoint = nullptr;
		SpvReflectEntryPoint* m_missEntryPoint = nullptr;
		SpvReflectEntryPoint* m_closestHitEntryPoint = nullptr;
		SpvReflectEntryPoint* m_anyHitEntryPoint = nullptr;
		SpvReflectEntryPoint* m_intersectionEntryPoint = nullptr;

		VkPipelineLayout m_pipelineLayout;
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayout;

		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
		SpvReflectInterfaceVariable* m_shaderRecord;
		std::vector<ShaderRecordVariableData> m_shaderRecordVariables;
	};
}