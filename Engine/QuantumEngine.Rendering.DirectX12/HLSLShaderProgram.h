#pragma once
#include "Rendering\ShaderProgram.h"
#include "HLSLShader.h"
#include "Core/HLSLReflection.h"
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
        std::map<UInt32, HLSLConstantBufferData> constantBufferVariables;
        std::map<UInt32, BoundResourceData> boundResourceDatas;
        UInt32 RootParameterCount;
        UInt32 totalVariableSize;
    };

    class HLSLShaderProgram : public ShaderProgram
    {
    public:
        HLSLShaderProgram() = default;
        
        HLSLProgramReflection* GetReflectionData1() { return &m_reflectionData; }
        bool Initialize(const ComPtr<ID3D12Device10>& device, bool isLocal);

        /// <summary>
        /// Gets the parameter layout for this shader program
        /// </summary>
        /// <returns></returns>
        inline HLSLReflection* GetReflectionData() { return &m_reflection; }

        /// <summary>
        /// Gets the root signature for this shader program
        /// </summary>
        /// <returns></returns>
        inline ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_rootSignature; }

        virtual bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device) = 0;
    private:
        HLSLProgramReflection m_reflectionData;

    protected:
        HLSLReflection m_reflection;
        ComPtr<ID3D12RootSignature> m_rootSignature;
    };
}
