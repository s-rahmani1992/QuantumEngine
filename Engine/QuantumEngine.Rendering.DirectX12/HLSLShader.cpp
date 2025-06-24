#include "pch.h"
#include "HLSLShader.h"

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12ShaderReflection>& reflection)
	:Shader(byteCode, codeLength), m_shaderType(shaderType)
{
    D3D12_SHADER_DESC desc;
    reflection->GetDesc(&desc);

    for (UINT i = 0; i < desc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC boundResource;
        reflection->GetResourceBindingDesc(i, &boundResource);

        if (boundResource.Type == D3D_SIT_CBUFFER) // Constant Buffer
        {
            ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByName(boundResource.Name);
            D3D12_SHADER_BUFFER_DESC cbDesc;
            cb->GetDesc(&cbDesc);
            
            for (UINT v = 0; v < cbDesc.Variables; ++v) {
                ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(v);
                D3D12_SHADER_VARIABLE_DESC varDesc;
                var->GetDesc(&varDesc);
                m_reflection.rootConstants.push_back(HLSLRootConstantData{
                    .name = std::string(varDesc.Name),
                    .variableDesc = varDesc,
                    .registerData = D3D12_ROOT_CONSTANTS{
                        .ShaderRegister = boundResource.BindPoint,
                        .RegisterSpace = boundResource.Space,
                        .Num32BitValues = varDesc.Size / 4,
                    },
                });
            }
        }
    }
}
