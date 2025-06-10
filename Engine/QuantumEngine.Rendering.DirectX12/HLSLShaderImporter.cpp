#include "pch.h"
#include "HLSLShader.h"
#include "HLSLShaderImporter.h"

ref<QuantumEngine::Rendering::Shader> QuantumEngine::Rendering::DX12::HLSLShaderImporter::Import(const std::wstring& fileName, const DX12_Shader_Type& shaderType, std::string& error)
{
	Microsoft::WRL::ComPtr<ID3DBlob> compiledCode;
	Microsoft::WRL::ComPtr<ID3DBlob> errorCode;

	std::string shaderStr;

	switch (shaderType) {
	case DX12_Shader_Type::VERTEX_SHADER :
		shaderStr = "vs_5_1";
		break;
	case DX12_Shader_Type::PIXEL_SHADER:
		shaderStr = "ps_5_1";
		break;
	}

	if (FAILED(D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", shaderStr.c_str(), 0, 0, &compiledCode, &errorCode))) {
		error = std::string((const char*)errorCode->GetBufferPointer());
		return nullptr;
	}

    return std::make_shared<HLSLShader>((Byte*)compiledCode->GetBufferPointer(), compiledCode->GetBufferSize(), shaderType);
}
