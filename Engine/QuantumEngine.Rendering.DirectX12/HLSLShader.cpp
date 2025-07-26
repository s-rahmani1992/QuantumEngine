#include "pch.h"
#include "HLSLShader.h"
#include "StringUtilities.h"

UInt32 QuantumEngine::Rendering::DX12::HLSLShader::m_shaderCounter = 0;

QuantumEngine::Rendering::DX12::HLSLShader::HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType)
    :Shader(byteCode, codeLength, shaderType), m_shaderType(shaderType)
{
    m_shaderCounter++;
}

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

D3D12_HIT_GROUP_DESC QuantumEngine::Rendering::DX12::HLSLShader::GetHitGroup(const std::wstring& exportName)
{
    D3D12_HIT_GROUP_DESC hitGroup;
    hitGroup.IntersectionShaderImport = m_entryPoints.contains(INTERSECTION_NAME) ? m_entryPoints[INTERSECTION_NAME].c_str() : nullptr;
    hitGroup.AnyHitShaderImport = m_entryPoints.contains(ANYHIT_NAME) ? m_entryPoints[ANYHIT_NAME].c_str() : nullptr;
    hitGroup.ClosestHitShaderImport = m_entryPoints.contains(CLOSEHIT_NAME) ? m_entryPoints[CLOSEHIT_NAME].c_str() : nullptr;
    hitGroup.HitGroupExport = exportName.c_str();
    hitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    return hitGroup;
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
        std::wstring methodName = CharToString(g.substr(2, lastIndex - 2).c_str());
        m_entryPoints.emplace(methodName, L"shader_" + std::to_wstring(m_shaderCounter) + L"_" + methodName);

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

    InitializeDXIL();
}

void QuantumEngine::Rendering::DX12::HLSLShader::InitializeDXIL()
{
    m_exportDescs.reserve(m_entryPoints.size());

    // Create DXIL for local material
    for (auto& entry : m_entryPoints)
    {
        m_exportDescs.push_back(D3D12_EXPORT_DESC{
            .Name = entry.second.c_str(),
            .ExportToRename = entry.first.c_str(),
            .Flags = D3D12_EXPORT_FLAG_NONE,
            });
    }

    m_dxilData.DXILLibrary = D3D12_SHADER_BYTECODE{
        .pShaderBytecode = GetByteCode(),
        .BytecodeLength = GetCodeSize(),
    };
    m_dxilData.NumExports = m_entryPoints.size();
    m_dxilData.pExports = m_exportDescs.data();
}
