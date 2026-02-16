#pragma once

#include "vulkan-pch.h"
#include "Rendering/ShaderProgram.h"

namespace QuantumEngine::Rendering::Vulkan {
	class SPIRVShader;
}

namespace QuantumEngine::Rendering::Vulkan::Rasterization {
	class SPIRVRasterizationProgram : public ShaderProgram
	{
	public:
		SPIRVRasterizationProgram(const std::vector<ref<SPIRVShader>>& spirvShaders);
		virtual ~SPIRVRasterizationProgram() = default;

	private:
		ref<SPIRVShader> m_vertexShader;
		ref<SPIRVShader> m_geometryShader;
		ref<SPIRVShader> m_pixelShader;
	};
}