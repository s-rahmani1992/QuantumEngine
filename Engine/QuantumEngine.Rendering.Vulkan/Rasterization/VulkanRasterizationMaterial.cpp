#include "vulkan-pch.h"
#include "VulkanRasterizationMaterial.h"
#include "Rendering/Material.h"
#include "SPIRVRasterizationProgram.h"

QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::VulkanRasterizationMaterial(const ref<Material>& material)
	:m_material(material), m_program(std::dynamic_pointer_cast<SPIRVRasterizationProgram>(material->GetProgram()))
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

void QuantumEngine::Rendering::Vulkan::Rasterization::VulkanRasterizationMaterial::BindValues(VkCommandBuffer commandBuffer)
{
	for(auto& pushConstant : m_pushConstantValues)
	{
		vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, pushConstant.offset, pushConstant.size, pushConstant.location);
	}
}
