#pragma once
#include "Rendering/Shader.h"
#include <vector>
#include <string>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	enum DX12_Shader_Type { //TODO If possible, convert to enum class
		VERTEX_SHADER = 0,
		PIXEL_SHADER = 1,
	};

	struct HLSLRootConstantData {
		std::string name;
		D3D12_SHADER_VARIABLE_DESC variableDesc;
		D3D12_ROOT_CONSTANTS registerData;
	};

	struct HLSLShaderReflection {
		std::vector<HLSLRootConstantData> rootConstants;
		std::vector<std::pair<std::string, D3D12_SHADER_INPUT_BIND_DESC>> boundVariables;
	};

	class HLSLShader : public Shader {
	public:
		HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12ShaderReflection>& reflection);
		HLSLShaderReflection* GetReflection() { return &m_reflection; }
	private:
		DX12_Shader_Type m_shaderType;
		HLSLShaderReflection m_reflection;
	};
}