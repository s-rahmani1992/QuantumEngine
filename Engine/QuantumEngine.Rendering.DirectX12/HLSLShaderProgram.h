#pragma once
#include "Rendering\ShaderProgram.h"
#include "HLSLShader.h"
#include <map>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
    struct BoundResourceData {
        UInt32 rootParameterIndex;
        std::string name;
        D3D12_SHADER_INPUT_BIND_DESC resourceData;
    };

    struct HLSLProgramReflection {
        ComPtr<ID3D12RootSignature> rootSignature;
        std::vector<std::pair<UInt32, HLSLRootConstantData>> rootConstants;
        std::vector<BoundResourceData> boundResourceDatas;
    };

    class HLSLShaderProgram : public ShaderProgram
    {
    public:
        HLSLShaderProgram(const std::initializer_list<ref<Shader>>& shaders)
            :ShaderProgram(shaders)
        {

        }
        HLSLProgramReflection* GetReflectionData() { return &m_reflectionData; }
        bool Initialize(const ComPtr<ID3D12Device10>& device, bool isLocal);
    private:
        HLSLProgramReflection m_reflectionData;
    };
}
