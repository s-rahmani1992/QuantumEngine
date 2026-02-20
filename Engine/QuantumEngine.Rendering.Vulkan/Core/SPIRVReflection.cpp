#include "vulkan-pch.h"
#include "SPIRVReflection.h"
#include <algorithm>

void QuantumEngine::Rendering::Vulkan::SPIRVReflection::AddShaderReflection(const SpvReflectShaderModule* shaderReflectionModule)
{
	UInt32 pc_count = 0;
	spvReflectEnumeratePushConstantBlocks(shaderReflectionModule, &pc_count, nullptr);
	std::vector<SpvReflectBlockVariable*> pushConstants(pc_count);
	spvReflectEnumeratePushConstantBlocks(shaderReflectionModule, &pc_count, pushConstants.data());

	for(auto& pushConstant : pushConstants)
	{
		auto it = std::find_if(m_pushConstants.begin(), m_pushConstants.end(),
			[pushConstant](const PushConstantBufferData& data) { return data.name.compare(pushConstant->name) == 0; });
		
		if(it != m_pushConstants.end())//checking if the resource is already processed because it can exist in multiple shaders
			continue;

		auto& pushConstantItem = m_pushConstants.emplace_back(PushConstantBufferData{
			.name = pushConstant->name,
			.data = *pushConstant,
			});

		pushConstantItem.variables.reserve(pushConstant->member_count);

		for (UInt32 i = 0; i < pushConstant->member_count; ++i) {
			auto& member = pushConstant->members[i];
			pushConstantItem.variables.push_back(PushConstantVariableData{
				.name = member.name,
				.variableDesc = member,
			});
		}
	}
}

VkPipelineLayout QuantumEngine::Rendering::Vulkan::SPIRVReflection::CreatePipelineLayout(const VkDevice device)
{
	std::vector<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.reserve(m_pushConstants.size());

	for(auto& pushConstant : m_pushConstants)
	{
		pushConstantRanges.push_back(VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
			.offset = pushConstant.data.offset,
			.size = pushConstant.data.size,
		});
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = (UInt32)(pushConstantRanges.size()),
		.pPushConstantRanges = pushConstantRanges.data(), // Optional
	};

	VkPipelineLayout pipelineLayout = nullptr;
	vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
	return pipelineLayout;
}

QuantumEngine::Rendering::MaterialReflection QuantumEngine::Rendering::Vulkan::SPIRVReflection::CreateMaterialReflection()
{
	UInt32 fieldIndex = 0;
	MaterialReflection reflectionData;

	for (auto& pushConst : m_pushConstants) {
		if (pushConst.name[0] == '_') {// Skip internal root constants
			fieldIndex++;
			continue;
		}

		for (auto& rootVar : pushConst.variables) {
			MaterialValueFieldInfo valueFieldInfo{
				.name = rootVar.name,
				.fieldIndex = fieldIndex,
				.size = rootVar.variableDesc.size,
			};
			reflectionData.valueFields.push_back(valueFieldInfo);
			fieldIndex++;
		}
	}

	return reflectionData;
}
