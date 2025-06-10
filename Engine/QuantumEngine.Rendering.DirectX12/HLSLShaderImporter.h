#pragma once
#include "BasicTypes.h"
#include <string>

namespace QuantumEngine::Rendering::DX12 {
	class HLSLShader;
	enum class DX12_Shader_Type;

	class HLSLShaderImporter {
	public:
		static ref<Shader> Import(const std::wstring& fileName, const DX12_Shader_Type& shaderType, std::string& error);
	};
}