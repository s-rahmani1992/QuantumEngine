#include "pch.h"
#include "HLSLReflection.h"
#include <Core/Matrix4.h>

void QuantumEngine::Rendering::DX12::HLSLReflection::AddShaderReflection(ID3D12ShaderReflection* shaderReflection)
{
    D3D12_SHADER_DESC desc;
    shaderReflection->GetDesc(&desc);

    for (UINT i = 0; i < desc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC boundResource;
        shaderReflection->GetResourceBindingDesc(i, &boundResource);
        std::string name(boundResource.Name);

        // Constant Buffer is processed separately. Because it can be either a single buffer or collection of root constants.
        if (boundResource.Type == D3D_SIT_CBUFFER)
        {
            auto rootConstantIt = std::find_if(m_rootConstants.begin(), m_rootConstants.end(),
                [&name](const RootConstantBufferData& data) { return data.name == name; });

            if (rootConstantIt != m_rootConstants.end())
                continue; //checking if the resource is already processed because it can exist in multiple shaders

            auto recourceIt = std::find_if(m_resourceVariables.begin(), m_resourceVariables.end(),
                [&name](const ResourceVariableData& data) { return data.name == name; });

            if (recourceIt != m_resourceVariables.end())
                continue; //checking if the resource is already processed because it can exist in multiple shaders

            auto samplerIt = std::find_if(m_samplerVariables.begin(), m_samplerVariables.end(),
                [&name](const ResourceVariableData& data) { return data.name == name; });

            if (samplerIt != m_samplerVariables.end())
                continue; //checking if the resource is already processed because it can exist in multiple shaders

            ID3D12ShaderReflectionConstantBuffer* cb = shaderReflection->GetConstantBufferByName(boundResource.Name);
            D3D12_SHADER_BUFFER_DESC cbDesc;
            cb->GetDesc(&cbDesc);

            //if constant buffer size per variable is less than Matrix4, then it is considered as a holder of root constants
            if (cbDesc.Size / cbDesc.Variables <= sizeof(Matrix4)) { //TODO Find better condition for constant buffer checking if possible
                RootConstantBufferData cData{
                    .rootParameterIndex = m_rootparameterCount,
                    .name = name,
                    .resourceData = boundResource,
                    .registerData = D3D12_ROOT_CONSTANTS{
                        .ShaderRegister = boundResource.BindPoint,
                        .RegisterSpace = boundResource.Space,
                        .Num32BitValues = cbDesc.Size / 4,
                    },
                };

                for (UINT v = 0; v < cbDesc.Variables; ++v) {
                    ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(v);
                    D3D12_SHADER_VARIABLE_DESC varDesc;
                    var->GetDesc(&varDesc);

                    cData.rootConstants.push_back(RootConstantVariableData{
                        .name = std::string(varDesc.Name),
                        .variableDesc = varDesc,
                        });
                }

                m_rootConstants.push_back(cData);
            }
            else {
                m_resourceVariables.push_back(ResourceVariableData{
                    .rootParameterIndex = m_rootparameterCount,
                    .name = name,
                    .resourceData = boundResource });
            }

            m_rootparameterCount++;
        }
        // Sampler is considered as separate type because it can either contribute as root parameter or as static sampler
        else if (boundResource.Type == D3D_SIT_SAMPLER) {
            m_samplerVariables.push_back(ResourceVariableData{ .rootParameterIndex = 9999, .name = name, .resourceData = boundResource });
        }
        else {
            m_resourceVariables.push_back(ResourceVariableData{ .rootParameterIndex = m_rootparameterCount, .name = name, .resourceData = boundResource });
            m_rootparameterCount++;
        }
    }
}

ComPtr<ID3D12RootSignature> QuantumEngine::Rendering::DX12::HLSLReflection::CreateRootSignature(const ComPtr<ID3D12Device10>& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::string& errorStr)
{
    std::vector<D3D12_ROOT_PARAMETER> rootParameters(m_resourceVariables.size() + m_rootConstants.size());
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges(m_resourceVariables.size());
    UInt8 rangeIndex = 0;

    // all root constants belonging to the same buffer are added as a single root constant variable
    for (auto& rootConst : m_rootConstants) {
        rootParameters[rootConst.rootParameterIndex] = D3D12_ROOT_PARAMETER{
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants = rootConst.registerData,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        };
    }

    // samplers are added as static samplers for now with default settings
    for (auto& samplerVar : m_samplerVariables) {
        staticSamplers.push_back(D3D12_STATIC_SAMPLER_DESC{
            .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .MipLODBias = 0.0f,
            .MaxAnisotropy = 1,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            .MinLOD = 0.0f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = samplerVar.resourceData.BindPoint,
            .RegisterSpace = samplerVar.resourceData.Space,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            });
    }

    // all other resource variables are added as descriptor tables with single range for now
    // TODO: optimize by grouping resources of same type into single descriptor table with multiple ranges
    for (auto& resVar : m_resourceVariables) {
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType;
        switch (resVar.resourceData.Type) {
        case D3D_SIT_RTACCELERATIONSTRUCTURE:
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            break;
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            break;
        case D3D_SIT_CBUFFER:
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            break;
        default:
            break;
        }

        ranges[rangeIndex] = D3D12_DESCRIPTOR_RANGE{
        .RangeType = rangeType,
        .NumDescriptors = 1,
        .BaseShaderRegister = resVar.resourceData.BindPoint,
        .RegisterSpace = resVar.resourceData.Space,
        .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
        };

        rootParameters[resVar.rootParameterIndex] = D3D12_ROOT_PARAMETER{
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        .DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE{
            .NumDescriptorRanges = 1,
            .pDescriptorRanges = ranges.data() + rangeIndex,
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        };

        rangeIndex++;
    }

    // Create Root Signature
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = flag;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{
        .NumParameters = (UInt32)rootParameters.size(),
        .pParameters = rootParameters.size() == 0 ? nullptr : rootParameters.data(),
        .NumStaticSamplers = (UInt32)staticSamplers.size(),
        .pStaticSamplers = staticSamplers.size() == 0 ? nullptr : staticSamplers.data(),
        .Flags = rootSignatureFlag,
    };

    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> rootSignatureByteCode;
    ComPtr<ID3D12RootSignature> rootSignature;

    if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureByteCode, &error))) {

        errorStr = std::string((char*)error->GetBufferPointer(), error->GetBufferSize());
        return nullptr;
    }

    if (FAILED(device->CreateRootSignature(0, rootSignatureByteCode->GetBufferPointer()
        , rootSignatureByteCode->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) {
		errorStr = "Failed to create root signature.";
        return nullptr;
    }

    return rootSignature;
}
