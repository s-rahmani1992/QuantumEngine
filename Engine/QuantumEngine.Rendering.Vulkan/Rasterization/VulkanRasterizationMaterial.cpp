#include "vulkan-pch.h"
#include "VulkanRasterizationMaterial.h"
#include "Rendering/Material.h"
#include "SPIRVRasterizationProgram.h"

QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::VulkanRasterizationMaterial(const ref<Material>& material, const VkDevice device)
	:m_material(material), m_device(device), m_program(std::dynamic_pointer_cast<SPIRVRasterizationProgram>(material->GetProgram()))
{
	m_pipelineLayout = m_program->GetPipelineLayout();
	auto& reflection = m_program->GetReflection();
	auto& pushConstants = reflection.GetPushConstants();

	for(auto& pushConstant : pushConstants)
	{
		auto valueLocation = material->GetValueLocation(pushConstant.variables[0].name);
		if(valueLocation != nullptr)
		{
			m_pushConstantValues.push_back({
				.offset = pushConstant.data.offset,
				.location = valueLocation,
				.size = pushConstant.data.size,
			});
		}
	}

}

bool QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::Initialize(const VkDescriptorPool pool)
{
	auto& layouts = m_program->GetDiscriptorLayouts();
	m_descriptorSets.resize(layouts.size());

	VkDescriptorSetAllocateInfo descSetAlloc{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = nullptr,
	};

	for (UInt32 i = 0; i < layouts.size(); i++) {
		descSetAlloc.pSetLayouts = layouts.data() + i;

		if (vkAllocateDescriptorSets(m_device, &descSetAlloc, m_descriptorSets.data() + i) != VK_SUCCESS)
			return false;
	}
	
	return true;
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::BindValues(VkCommandBuffer commandBuffer)
{
	for(auto& pushConstant : m_pushConstantValues)
	{
		vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, pushConstant.offset, pushConstant.size, pushConstant.location);
	}
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::BindDynamicValues(VkCommandBuffer commandBuffer, UInt32* offsets, UInt32 offsetCount)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, (UInt32)m_descriptorSets.size(), m_descriptorSets.data(), offsetCount, offsets);
}

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::WriteBuffer(const std::string name, const VkBuffer buffer, UInt32 stride)
{
	auto descriptorData = m_program->GetReflection().GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	VkDescriptorBufferInfo descBufferInfo{
		.buffer = buffer,
		.offset = 0,
		.range = stride,
	};

	VkWriteDescriptorSet writeDescriptor{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptorSets[descriptorData->data.set],
		.dstBinding = descriptorData->data.binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorData->descriptorType,
		.pImageInfo = nullptr,
		.pBufferInfo = &descBufferInfo,
		.pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
}
