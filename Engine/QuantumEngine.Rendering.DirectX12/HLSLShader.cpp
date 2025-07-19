#include "pch.h"
#include "HLSLShader.h"

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType)
    :Shader(byteCode, codeLength, shaderType), m_shaderType(shaderType){ }

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12ShaderReflection>& shaderReflection)
	:HLSLShader(byteCode, codeLength, shaderType)
{
    FillReflection(shaderReflection);
}

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12LibraryReflection>& libraryReflection)
    :HLSLShader(byteCode, codeLength, shaderType)
{
    FillReflection(libraryReflection);
}

void QuantumEngine::Rendering::DX12::HLSLShader::FillReflection(ComPtr<ID3D12ShaderReflection>& shaderReflection) {
    D3D12_SHADER_DESC desc;
    shaderReflection->GetDesc(&desc);

    for (UINT i = 0; i < desc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC boundResource;
        shaderReflection->GetResourceBindingDesc(i, &boundResource);

        if (boundResource.Type == D3D_SIT_CBUFFER) // Constant Buffer
        {
            ID3D12ShaderReflectionConstantBuffer* cb = shaderReflection->GetConstantBufferByName(boundResource.Name);
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
        else {
            m_reflection.boundVariables.push_back({ std::string(boundResource.Name), boundResource });
        }
    }
}

void QuantumEngine::Rendering::DX12::HLSLShader::FillReflection(ComPtr<ID3D12LibraryReflection>& libraryReflection) {
    D3D12_LIBRARY_DESC libDesc;
    libraryReflection->GetDesc(&libDesc);

    for (int i = 0; i < libDesc.FunctionCount; i++) {
        ID3D12FunctionReflection* funcReflection = libraryReflection->GetFunctionByIndex(i);
        D3D12_FUNCTION_DESC funcDesc;
        funcReflection->GetDesc(&funcDesc);
        std::string g(funcDesc.Name);
        auto lastIndex = g.find_first_of('@');
        m_entryPoints.push_back(g.substr(2, lastIndex - 2));

        for (int r = 0; r < funcDesc.BoundResources; r++) {
            D3D12_SHADER_INPUT_BIND_DESC boundResource;
            funcReflection->GetResourceBindingDesc(r, &boundResource);

            if (boundResource.Type == D3D_SIT_CBUFFER) // Constant Buffer
            {
                ID3D12ShaderReflectionConstantBuffer* cb = funcReflection->GetConstantBufferByName(boundResource.Name);
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
            else {
                m_reflection.boundVariables.push_back({ std::string(boundResource.Name), boundResource });
            }
        } 
    }
}
