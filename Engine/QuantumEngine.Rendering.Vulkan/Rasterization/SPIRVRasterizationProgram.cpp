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
			m_reflection.AddShaderReflection(shader->GetReflectionModule());
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
			m_reflection.AddShaderReflection(shader->GetReflectionModule());
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
			m_reflection.AddShaderReflection(shader->GetReflectionModule());
		}
	}

	m_reflection.Initializes();
	InitializeSampler();

	m_descriptorSetLayout.resize(m_reflection.GetDescriptorLayoutCount());
	m_reflection.CreatePipelineLayout(device, m_sampler, &m_pipelineLayout, m_descriptorSetLayout.data());
}

QuantumEngine::Rendering::Vulkan::Rasterization::SPIRVRasterizationProgram::~SPIRVRasterizationProgram()
{
	vkDestroySampler(m_device, m_sampler, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	for(auto& descriptorLayout : m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(m_device, descriptorLayout, nullptr);
}

void QuantumEngine::Rendering::Vulkan::Rasterization::SPIRVRasterizationProgram::InitializeSampler()
{
	VkSamplerCreateInfo samplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 0,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	auto result = vkCreateSampler(m_device, &samplerCreateInfo, nullptr, &m_sampler);
}
