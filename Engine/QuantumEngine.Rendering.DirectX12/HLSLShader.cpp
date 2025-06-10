#include "pch.h"
#include "HLSLShader.h"

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType)
	:Shader(byteCode, codeLength), m_shaderType(shaderType)
{
	D3DGetBlobPart(byteCode, codeLength, D3D_BLOB_ROOT_SIGNATURE, 0, &m_rootSignature);
}
