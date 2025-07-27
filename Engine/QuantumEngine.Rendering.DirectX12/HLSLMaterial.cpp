#include "pch.h"
#include "HLSLMaterial.h"
#include "HLSLShader.h"
#include "HLSLShaderProgram.h"
#include "Core/Vector2.h"
#include "Core/Vector3.h"
#include "Core/Matrix4.h"
#include "DX12Texture2DController.h"
#include "Core/Texture2D.h"

QuantumEngine::Rendering::DX12::HLSLMaterial::HLSLMaterial(const ref<ShaderProgram>& program)
    :Material(program), m_variableData(nullptr)
{
}

QuantumEngine::Rendering::DX12::HLSLMaterial::~HLSLMaterial()
{
    if (m_variableData != nullptr)
        delete[] m_variableData;
}

bool QuantumEngine::Rendering::DX12::HLSLMaterial::Initialize()
{
    auto reflection = std::dynamic_pointer_cast<HLSLShaderProgram>(m_program)->GetReflectionData();
    
    m_variableData = new Byte[reflection->totalVariableSize];
    Byte* startPoint = m_variableData;

    for (int i = 0; i < reflection->RootParameterCount; i++) {
        auto cBufferIt = reflection->constantBufferVariables.find(i);
        if (cBufferIt != reflection->constantBufferVariables.end()) {
            m_constantRegisterValues.push_back(constantBufferData{
                .rootParamIndex = (*cBufferIt).first,
                .location = startPoint,
                .size = (UInt32)((*cBufferIt).second.registerData.Num32BitValues),
                });

            for (auto& cVar : (*cBufferIt).second.constantBufferVariables) {
                m_rootConstantVariableLocations.emplace(cVar.name, startPoint);
                startPoint += cVar.variableDesc.Size;
            }

            continue;
        }

        auto heapIt = reflection->boundResourceDatas.find(i);
        if (heapIt != reflection->boundResourceDatas.end()) {
            m_heapValues.emplace((*heapIt).second.name, HeapData{
                .rootParamIndex = (*heapIt).first,
                .gpuHandle = (D3D12_GPU_DESCRIPTOR_HANDLE*)startPoint, 
                });
            startPoint += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
            continue;
        }

        return false;
    }

	return true;
}

std::map<std::string, D3D12_SHADER_VARIABLE_DESC> QuantumEngine::Rendering::DX12::HLSLMaterial::ExtractConstantBuffers(const ComPtr<ID3D12ShaderReflection>& reflector)
{
    std::map<std::string, D3D12_SHADER_VARIABLE_DESC> constantBuffers;

    D3D12_SHADER_DESC desc;
    reflector->GetDesc(&desc);

    for (UINT i = 0; i < desc.ConstantBuffers; ++i) {
        ID3D12ShaderReflectionConstantBuffer* cb = reflector->GetConstantBufferByIndex(i);
        D3D12_SHADER_BUFFER_DESC cbDesc;
        cb->GetDesc(&cbDesc);

        for (UINT v = 0; v < cbDesc.Variables; ++v) {
            ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(v);
            D3D12_SHADER_VARIABLE_DESC varDesc;
            var->GetDesc(&varDesc);
            constantBuffers.insert({ std::string(varDesc.Name), varDesc });
        }
    }

    return constantBuffers;
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetRootConstant(const std::string& fieldName, void* data, UInt32 size)
{
    auto field = m_rootConstantVariableLocations.find(fieldName);

    if (field != m_rootConstantVariableLocations.end()) {
        std::memcpy((*field).second, data, size);
    }
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::RegisterValues(ComPtr<ID3D12GraphicsCommandList7>& commandList)
{
    for(auto& rField : m_constantRegisterValues)
        commandList->SetGraphicsRoot32BitConstants(rField.rootParamIndex, rField.size, rField.location, 0);

    for (auto& heapField : m_heapValues) {
        commandList->SetDescriptorHeaps(1, heapField.second.descriptor.GetAddressOf());
        commandList->SetGraphicsRootDescriptorTable(heapField.second.rootParamIndex, *heapField.second.gpuHandle);
    }
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::RegisterComputeValues(ComPtr<ID3D12GraphicsCommandList7>& commandList)
{
    for (auto& rField : m_constantRegisterValues)
        commandList->SetComputeRoot32BitConstants(rField.rootParamIndex, rField.size, rField.location, 0);

    for (auto& heapField : m_heapValues) {
        commandList->SetDescriptorHeaps(1, heapField.second.descriptor.GetAddressOf());
        commandList->SetComputeRootDescriptorTable(heapField.second.rootParamIndex, *heapField.second.gpuHandle);
    }
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetColor(const std::string& fieldName, const Color& color)
{
    SetRootConstant(fieldName, (void*)&color, sizeof(Color));
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetFloat(const std::string& fieldName, const Float& fValue)
{
    SetRootConstant(fieldName, (void*)&fValue, sizeof(Float));
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetVector2(const std::string& fieldName, const Vector2& fValue)
{
    SetRootConstant(fieldName, (void*)&fValue, sizeof(Vector2));
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetVector3(const std::string& fieldName, const Vector3& fValue)
{
    SetRootConstant(fieldName, (void*)&fValue, sizeof(Vector3));
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetMatrix(const std::string& fieldName, const Matrix4& matrixValue)
{
    SetRootConstant(fieldName, (void*)&matrixValue, sizeof(Matrix4));
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetTexture2D(const std::string& fieldName, const ref<Texture2D>& texValue)
{
    SetDescriptorHeap(fieldName, std::dynamic_pointer_cast<DX12Texture2DController>(texValue->GetGPUHandle())->GetShaderView());
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetDescriptorHeap(const std::string& fieldName, const ComPtr<ID3D12DescriptorHeap>& descriptorHeap)
{
    auto field = m_heapValues.find(fieldName);

    if (field != m_heapValues.end()) {
        *((*field).second.gpuHandle) = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        (*field).second.descriptor = descriptorHeap;
    }
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::CopyVariableData(void* dest)
{
    std::memcpy(dest, m_variableData, std::dynamic_pointer_cast<HLSLShaderProgram>(m_program)->GetReflectionData()->totalVariableSize);
}
