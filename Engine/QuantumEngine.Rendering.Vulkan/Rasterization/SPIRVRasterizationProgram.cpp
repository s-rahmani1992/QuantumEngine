#include "vulkan-pch.h"
#include "SPIRVRasterizationProgram.h"
#include "../Core/SPIRVShader.h"

QuantumEngine::Rendering::Vulkan::Rasterization::SPIRVRasterizationProgram::SPIRVRasterizationProgram(const std::vector<ref<SPIRVShader>>& spirvShaders, const VkDevice device)
	:m_device(device)
{
	for(auto& shader : spirvShaders)
	{
		if (shader->GetShaderType() == Vulkan_Vertex) {
			m_vertexShader = shader;
			m_stageInfos.push_back({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = shader->GetShaderModule(),
				.pName = shader->GetEntryPoint().c_str(),
				});
		}
		else if (shader->GetShaderType() == Vulkan_Vertex) {
			m_geometryShader = shader;
			m_stageInfos.push_back({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_GEOMETRY_BIT,
				.module = shader->GetShaderModule(),
				.pName = shader->GetEntryPoint().c_str(),
				});
		}
		else if (shader->GetShaderType() == Vulkan_Fragment) {
			m_pixelShader = shader;
			m_stageInfos.push_back({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = shader->GetShaderModule(),
				.pName = shader->GetEntryPoint().c_str(),
				});
		}
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
}

QuantumEngine::Rendering::Vulkan::Rasterization::SPIRVRasterizationProgram::~SPIRVRasterizationProgram()
{
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}
