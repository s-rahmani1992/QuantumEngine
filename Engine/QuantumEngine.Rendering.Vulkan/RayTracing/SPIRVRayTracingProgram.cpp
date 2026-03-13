#include "vulkan-pch.h"
#include "SPIRVRayTracingProgram.h"

QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::SPIRVRayTracingProgram(Byte* bytecode, UInt64 codeSize, VkDevice device)
	:m_codeSize(codeSize)
{
	m_device = device;
	m_bytecode = new Byte[m_codeSize];
	std::memcpy(m_bytecode, bytecode, m_codeSize);

	VkShaderModuleCreateInfo shaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = codeSize,
		.pCode = reinterpret_cast<UInt32*>(m_bytecode),
	};

	vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &m_rtModule);

	spvReflectCreateShaderModule(m_codeSize, m_bytecode, &m_reflectionModule);
	m_reflection.AddShaderReflection(&m_reflectionModule);

	m_reflection.Initializes();
	InitializeSampler();


	auto shaderStageFlags = 0;

	for(UInt32 i = 0; i < m_reflectionModule.entry_point_count; i++) {
		auto& entryPoint = m_reflectionModule.entry_points[i];
		
		switch(entryPoint.spirv_execution_model) {
			case SpvExecutionModelRayGenerationKHR:
				m_rayGenEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				break;
			case SpvExecutionModelMissKHR:
				m_missEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_MISS_BIT_KHR;
				break;
			case SpvExecutionModelClosestHitKHR:
				m_closestHitEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				break;
			case SpvExecutionModelAnyHitKHR:
				m_anyHitEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				break;
			case SpvExecutionModelIntersectionKHR:
				m_intersectionEntryPoint = &entryPoint;
				shaderStageFlags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				break;
		}
	}

	m_descriptorSetLayout.resize(m_reflection.GetDescriptorLayoutCount());
	m_reflection.CreatePipelineLayout(m_device, shaderStageFlags, m_sampler, &m_pipelineLayout, m_descriptorSetLayout.data());


	int h = 0;
}

QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::~SPIRVRayTracingProgram()
{
	vkDestroySampler(m_device, m_sampler, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	for (auto& descriptorLayout : m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(m_device, descriptorLayout, nullptr);

	vkDestroyShaderModule(m_device, m_rtModule, nullptr);

	delete[] m_bytecode;
}

std::vector<VkPipelineShaderStageCreateInfo> QuantumEngine::Rendering::Vulkan::RayTracing::SPIRVRayTracingProgram::GetShaderStages()
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	if(m_rayGenEntryPoint) {
		VkPipelineShaderStageCreateInfo rayGenStage{};
		rayGenStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rayGenStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		rayGenStage.module = m_rtModule;
		rayGenStage.pName = m_rayGenEntryPoint->name;
		stages.push_back(rayGenStage);
	}
	if(m_missEntryPoint) {
		VkPipelineShaderStageCreateInfo missStage{};
		missStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		missStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		missStage.module = m_rtModule;
		missStage.pName = m_missEntryPoint->name;
		stages.push_back(missStage);
	}
	if(m_closestHitEntryPoint) {
		VkPipelineShaderStageCreateInfo closestHitStage{};
		closestHitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		closestHitStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		closestHitStage.module = m_rtModule;
		closestHitStage.pName = m_closestHitEntryPoint->name;
		stages.push_back(closestHitStage);
	}
	if(m_anyHitEntryPoint) {
		VkPipelineShaderStageCreateInfo anyHitStage{};
		anyHitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		anyHitStage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		anyHitStage.module = m_rtModule;
		anyHitStage.pName = m_anyHitEntryPoint->name;
		stages.push_back(anyHitStage);
	}
	if(m_intersectionEntryPoint) {
		VkPipelineShaderStageCreateInfo intersectionStage{};
		intersectionStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		intersectionStage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		intersectionStage.module = m_rtModule;
		intersectionStage.pName = m_intersectionEntryPoint->name;
		stages.push_back(intersectionStage);
	}
	return stages;
}
