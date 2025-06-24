#include "pch.h"
#include "HLSLMaterial.h"
#include "HLSLShader.h"
#include "HLSLShaderProgram.h"
#include "Core/Vector2.h"

QuantumEngine::Rendering::DX12::HLSLMaterial::HLSLMaterial(const ref<ShaderProgram>& program)
    :Material(program)
{
}

bool QuantumEngine::Rendering::DX12::HLSLMaterial::Initialize()
{
    auto reflection = std::dynamic_pointer_cast<HLSLShaderProgram>(m_program)->GetReflectionData();
    
    for (auto& p : reflection->rootConstants) {
        if (p.second.registerData.Num32BitValues == 1) // it's float
            m_floatValues.insert_or_assign(p.second.name, RootConstantData<Float>{
                .rootParamIndex = p.first,
                .offset = p.second.variableDesc.StartOffset / 4,
                .size = 1,
                .value = 0.0f,
            });

        else if (p.second.registerData.Num32BitValues == 2) // it's vector2
            m_vector2Values.insert_or_assign(p.second.name, RootConstantData<Vector2>{
                .rootParamIndex = p.first,
                .offset = p.second.variableDesc.StartOffset / 4,
                .size = 2,
                .value = Vector2(),
            });
        else if (p.second.registerData.Num32BitValues == 4) // it's color
            m_colorValues.insert_or_assign(p.second.name, RootConstantData<Color>{
                .rootParamIndex = p.first,
                .offset = p.second.variableDesc.StartOffset / 4,
                .size = 4,
                .value = Color(),
            });
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

void QuantumEngine::Rendering::DX12::HLSLMaterial::RegisterValues(ComPtr<ID3D12GraphicsCommandList7>& commandList)
{
    for (auto& colorField : m_colorValues)
        commandList->SetGraphicsRoot32BitConstants(colorField.second.rootParamIndex, 4, colorField.second.value.GetColorArray(), colorField.second.offset);
    for (auto& floatField : m_floatValues)
        commandList->SetGraphicsRoot32BitConstants(floatField.second.rootParamIndex, 1, &(floatField.second.value), floatField.second.offset);
    for (auto& vector2Field : m_vector2Values)
        commandList->SetGraphicsRoot32BitConstants(vector2Field.second.rootParamIndex, 2, &vector2Field.second.value, vector2Field.second.offset);
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetColor(const std::string& fieldName, const Color& color)
{
    auto field = m_colorValues.find(fieldName);

    if (field != m_colorValues.end()) {
        (*field).second.value = color;
    }
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetFloat(const std::string& fieldName, const Float& fValue)
{
    auto field = m_floatValues.find(fieldName);

    if (field != m_floatValues.end()) {
        (*field).second.value = fValue;
    }
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetVector2(const std::string& fieldName, const Vector2& fValue)
{
    auto field = m_vector2Values.find(fieldName);

    if (field != m_vector2Values.end()) {
        (*field).second.value = fValue;
    }
}
