#include "vulkan-pch.h"
#include "SPIRVRasterizationProgram.h"
#include "../Core/SPIRVShader.h"

QuantumEngine::Rendering::Vulkan::Rasterization::SPIRVRasterizationProgram::SPIRVRasterizationProgram(const std::vector<ref<SPIRVShader>>& spirvShaders)
{
	for(auto& shader : spirvShaders)
	{
		if(shader->GetShaderType() == Vulkan_Vertex)
			m_vertexShader = shader;
		else if(shader->GetShaderType() == Vulkan_Vertex)
			m_geometryShader = shader;
		else if(shader->GetShaderType() == Vulkan_Fragment)
			m_pixelShader = shader;
	}
}
