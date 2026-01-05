#include "pch.h"
#include "HLSLRasterizationProgram.h"
#include "HLSLShader.h"
#include <set>
#include <Core/Matrix4.h>

QuantumEngine::Rendering::DX12::Shader::HLSLRasterizationProgram::HLSLRasterizationProgram(const std::vector<ref<HLSLShader>>& shaders)
{
    std::set<std::string> keys;
    UInt8 rootParamIndex = 0;

	for (auto& shader : shaders) {
        // Set Shader Stages
		if (shader->GetShaderTypeId() == VERTEX_SHADER) {
			m_vertexShader = shader;
		}
		else if (shader->GetShaderTypeId() == GEOMETRY_SHADER) {
            m_geometryShader = shader;
		}
		else if (shader->GetShaderTypeId() == PIXEL_SHADER) {
			m_pixelShader = shader;
		}

		// Create Reflection Data by merging all shader reflections
		auto shaderReflection = shader->GetRawReflection();	
        D3D12_SHADER_DESC desc;
        shaderReflection->GetDesc(&desc);

        for (UINT i = 0; i < desc.BoundResources; ++i) {
            D3D12_SHADER_INPUT_BIND_DESC boundResource;
            shaderReflection->GetResourceBindingDesc(i, &boundResource);
            std::string name(boundResource.Name);
            auto resultItem = keys.emplace(name);

			if (resultItem.second == false) //checking if the resource is already processed because it can exist in multiple shaders
                continue;

            // Constant Buffer is processed separately. Because it can be either a single buffer or collection of root constants.
            if (boundResource.Type == D3D_SIT_CBUFFER)
            {
                ID3D12ShaderReflectionConstantBuffer* cb = shaderReflection->GetConstantBufferByName(boundResource.Name);
                D3D12_SHADER_BUFFER_DESC cbDesc;
                cb->GetDesc(&cbDesc);

				//if constant buffer size per variable is less than Matrix4, then it is considered as a holder of root constants
                if (cbDesc.Size / cbDesc.Variables <= sizeof(Matrix4)) { //TODO Find better condition for constant buffer checking if possible
                    RootConstantBufferData cData{
                        .rootParameterIndex = rootParamIndex,
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

					m_reflection.rootConstants.push_back(cData);
                }
                else {
                    m_reflection.resourceVariables.push_back(ResourceVariableData{ 
                        .rootParameterIndex = rootParamIndex, 
                        .name = name, 
                        .resourceData = boundResource });
                }

                rootParamIndex++;
            }
            // Sampler is considered as separate type because it can either contribute as root parameter or as static sampler
			else if (boundResource.Type == D3D_SIT_SAMPLER) {
                m_reflection.samplerVariables.push_back(ResourceVariableData{ .rootParameterIndex = 9999, .name = name, .resourceData = boundResource });
            }
            else {
				m_reflection.resourceVariables.push_back(ResourceVariableData{ .rootParameterIndex = rootParamIndex, .name = name, .resourceData = boundResource });
                rootParamIndex++;
            }
        }
	}
}

bool QuantumEngine::Rendering::DX12::Shader::HLSLRasterizationProgram::InitializeRootSignature(const ComPtr<ID3D12Device10>& device)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParameters(m_reflection.resourceVariables.size() + m_reflection.rootConstants.size());
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;    
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges(m_reflection.resourceVariables.size());
    UInt8 rangeIndex = 0;

    // all root constants belonging to the same buffer are added as a single root constant variable
    for (auto& rootConst : m_reflection.rootConstants) {
        rootParameters[rootConst.rootParameterIndex] = D3D12_ROOT_PARAMETER{
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants = rootConst.registerData,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
            };
	}

	// samplers are added as static samplers for now with default settings
    for(auto& samplerVar : m_reflection.samplerVariables) {
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
    for (auto& resVar : m_reflection.resourceVariables) {
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType;
        switch (resVar.resourceData.Type) {
        case D3D_SIT_RTACCELERATIONSTRUCTURE:
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
            rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            break;
        case D3D_SIT_UAV_RWTYPED:
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
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{
        .NumParameters = (UInt32)rootParameters.size(),
        .pParameters = rootParameters.size() == 0 ? nullptr : rootParameters.data(),
        .NumStaticSamplers = (UInt32)staticSamplers.size(),
        .pStaticSamplers = staticSamplers.size() == 0 ? nullptr : staticSamplers.data(),
        .Flags = rootSignatureFlag,
    };

    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> rootSignatureByteCode;

    if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureByteCode, &error))) {

        std::string g((char*)error->GetBufferPointer(), error->GetBufferSize());
        return false;
    }
    if (FAILED(device->CreateRootSignature(0, rootSignatureByteCode->GetBufferPointer()
        , rootSignatureByteCode->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature))))
        return false;

    return true;
}
