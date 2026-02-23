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

	// Extract Descriptors
	uint32_t binding_count = 0;
	spvReflectEnumerateDescriptorBindings(shaderReflectionModule, &binding_count, nullptr);
	
	std::vector<SpvReflectDescriptorBinding*> descriptorBindings(binding_count);
	spvReflectEnumerateDescriptorBindings(shaderReflectionModule, &binding_count, descriptorBindings.data());
	
	for (auto& descriptor : descriptorBindings) {
		auto it = std::find_if(m_descripters.begin(), m_descripters.end(),
			[descriptor](const DescriptableBufferData& data) { return data.name.compare(descriptor->name) == 0; });

		if (it != m_descripters.end())//checking if the resource is already processed because it can exist in multiple shaders
			continue;

		VkDescriptorType desType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		switch (descriptor->descriptor_type) {
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			desType = descriptor->name[0] != '_' ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			break;
		}

		m_descripters.push_back(DescriptableBufferData{
			.name = std::string(descriptor->name),
			.offsetIndex = UINT32_MAX,
			.descriptorType = desType,
			.data = *descriptor,
		});
	}

}

UInt32 QuantumEngine::Rendering::Vulkan::SPIRVReflection::GetDescriptorLayoutCount()
{
	auto h = std::max_element(m_descripters.begin(), m_descripters.end()
		, [](const DescriptableBufferData& desc1, const DescriptableBufferData& desc2) {
			return desc1.data.set < desc2.data.set;
		});

	return (*h).data.set + 1;
}

UInt32 QuantumEngine::Rendering::Vulkan::SPIRVReflection::GetDynamicDescriptorCount()
{
	auto h = std::count_if(m_descripters.begin(), m_descripters.end(), [](const DescriptableBufferData& desc) {
		return desc.name[0] == '_';
		});
	return UInt32(h);
}

void QuantumEngine::Rendering::Vulkan::SPIRVReflection::CreatePipelineLayout(const VkDevice device, VkPipelineLayout* pipelineLayout, VkDescriptorSetLayout* descriptorSetLayout)
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

	VkDescriptorType desType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	std::vector<std::vector<VkDescriptorSetLayoutBinding>> descriptorLayoutBindings(GetDescriptorLayoutCount());
	
	for (auto& descriptor : m_descripters) {
		descriptorLayoutBindings[descriptor.data.set].push_back(VkDescriptorSetLayoutBinding{
			.binding = descriptor.data.binding,
			.descriptorType = descriptor.descriptorType,
			.descriptorCount = descriptor.data.count,
			.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
			.pImmutableSamplers = nullptr,
		});
	}

	for (UInt32 i = 0; i < descriptorLayoutBindings.size(); i++) {
		VkDescriptorSetLayoutCreateInfo descriptorCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = (UInt32)descriptorLayoutBindings[i].size(),
		.pBindings = descriptorLayoutBindings[i].data(),
		};

		vkCreateDescriptorSetLayout(device, &descriptorCreateInfo, nullptr, descriptorSetLayout + i);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = descriptorSetLayout,
		.pushConstantRangeCount = (UInt32)(pushConstantRanges.size()),
		.pPushConstantRanges = pushConstantRanges.data(),
	};

	vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout);
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

QuantumEngine::Rendering::Vulkan::DescriptableBufferData* QuantumEngine::Rendering::Vulkan::SPIRVReflection::GetDescriptorData(const std::string name)
{
	auto it = std::find_if(m_descripters.begin(), m_descripters.end(), [name](const DescriptableBufferData& desc) {
		return desc.name == name;
		});

	if (it == m_descripters.end())
		return nullptr;

	return &(*it);
}

void QuantumEngine::Rendering::Vulkan::SPIRVReflection::Initializes()
{
	auto h = std::max_element(m_descripters.begin(), m_descripters.end()
		, [](const DescriptableBufferData& desc1, const DescriptableBufferData& desc2) {
			return desc1.data.set < desc2.data.set;
		});

	std::vector<UInt32> offsets((*h).data.set + 1, 0);
	std::vector<UInt32> finalOffsets((*h).data.set + 1, 0);

	UInt32 offsetIndex = UINT32_MAX;

	for (auto& descriptor : m_descripters) {
		if (descriptor.name[0] != '_')
			continue;

		descriptor.offsetIndex = offsets[descriptor.data.set];
		offsets[descriptor.data.set]++;
	}

	for (int i = 1; i < finalOffsets.size(); i++)
		finalOffsets[i] = finalOffsets[i - 1] + offsets[i - 1];

	for (auto& descriptor : m_descripters) {

		if (descriptor.name[0] != '_')
			continue;

		descriptor.offsetIndex += finalOffsets[descriptor.data.set];
	}
}
