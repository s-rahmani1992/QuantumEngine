#include "pch.h"
#include "HLSLShader.h"
#include "HLSLShaderProgram.h"
#include <set>

bool QuantumEngine::Rendering::DX12::HLSLShaderProgram::Initialize(const ComPtr<ID3D12Device10>& device)
{
    UInt32 rootParameterIndex = 0;
    std::set<std::string> keys;
    std::map<std::pair<UInt32, UInt32>, UInt32> bufferSizes;
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;

    for (auto& shader : m_shaders) {
        auto hlslShader = std::dynamic_pointer_cast<HLSLShader>(shader);
        auto reflection = hlslShader->GetReflection();
        for (auto& shaderRootConstant : reflection->rootConstants) {
            auto resultItem = keys.emplace(shaderRootConstant.name);
            
            if (resultItem.second == false)
                continue;

            int targetIndex = 0;

            for (targetIndex = 0; targetIndex < rootParameters.size(); targetIndex++) {
                if (rootParameters[targetIndex].Constants.RegisterSpace == shaderRootConstant.registerData.RegisterSpace
                    && rootParameters[targetIndex].Constants.ShaderRegister == shaderRootConstant.registerData.ShaderRegister)
                    break;
            }
            
            if (targetIndex != rootParameters.size()) {
                rootParameters[targetIndex].Constants.Num32BitValues += shaderRootConstant.registerData.Num32BitValues;
                m_reflectionData.rootConstants.push_back({ targetIndex, shaderRootConstant });
            }
            else {
                rootParameters.push_back(D3D12_ROOT_PARAMETER{
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                    .Constants = shaderRootConstant.registerData,
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                    });

                m_reflectionData.rootConstants.push_back({ rootParameterIndex, shaderRootConstant });
                rootParameterIndex++;
            }
        }
    }

    D3D12_ROOT_SIGNATURE_DESC rootDesc{
        .NumParameters = (UInt32)rootParameters.size(),
        .pParameters = rootParameters.size() == 0 ? nullptr : rootParameters.data(),
        .NumStaticSamplers = 0,
        .pStaticSamplers = nullptr,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> rootSignatureByteCode;

    if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureByteCode, &error))) {
        
        std::string g((char*)error->GetBufferPointer(), error->GetBufferSize());
        return false;
    }
    if (FAILED(device->CreateRootSignature(0, rootSignatureByteCode->GetBufferPointer()
        , rootSignatureByteCode->GetBufferSize(), IID_PPV_ARGS(&m_reflectionData.rootSignature))))
        return false;

    return true;
}
