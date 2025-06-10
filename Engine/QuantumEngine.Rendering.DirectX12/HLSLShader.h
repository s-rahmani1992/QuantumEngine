#pragma once
#include "Rendering/Shader.h"

namespace QuantumEngine::Rendering::DX12 {
	enum class DX12_Shader_Type {
		VERTEX_SHADER = 0,
		PIXEL_SHADER = 1,
	};

	class HLSLShader : public Shader {
	public:
		HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType);
		inline void* GetRootSignatureData() const { return m_rootSignature->GetBufferPointer(); }
		inline UInt64 GetRootSignatureLength() const { return m_rootSignature->GetBufferSize(); }
	private:
		DX12_Shader_Type m_shaderType;
		Microsoft::WRL::ComPtr<ID3DBlob> m_rootSignature;
	};
}