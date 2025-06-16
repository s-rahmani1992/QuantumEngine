#include "pch.h"
#include "HLSLMaterial.h"
#include "HLSLShader.h"
#include "Rendering/ShaderProgram.h"

QuantumEngine::Rendering::DX12::HLSLMaterial::HLSLMaterial(const ref<ShaderProgram>& program)
    :Material(program)
{
}

bool QuantumEngine::Rendering::DX12::HLSLMaterial::Initialize()
{
	auto vertexShader = std::dynamic_pointer_cast<HLSLShader>(m_program->GetShader(DX12::VERTEX_SHADER));
    ComPtr<ID3D12ShaderReflection> reflector;
    HRESULT hr = D3DReflect(
        vertexShader->GetByteCode(),
        vertexShader->GetCodeSize(),
        IID_PPV_ARGS(&reflector)
    );
    if (FAILED(hr)) {
        return false;
    }

    auto vertexConstants = ExtractConstantBuffers(reflector);

    for (auto& p : vertexConstants) {
        if (p.second.Size == 16) // it's color
            m_colorValues.insert_or_assign(std::string(p.second.Name), std::pair{ p.second, Color() });
    }

    auto pixelShader = std::dynamic_pointer_cast<HLSLShader>(m_program->GetShader(DX12::PIXEL_SHADER));

    hr = D3DReflect(
        pixelShader->GetByteCode(),
        pixelShader->GetCodeSize(),
        IID_PPV_ARGS(&reflector)
    );
    if (FAILED(hr)) {
        return false;
    }

    vertexConstants = ExtractConstantBuffers(reflector);

    for (auto& p : vertexConstants) {
        if (p.second.Size == 16) // it's color
        {
            m_colorValues.insert_or_assign(std::string(p.second.Name), std::pair{ p.second, Color() });
        }
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
        commandList->SetGraphicsRoot32BitConstants(0, 4, colorField.second.second.GetColorArray(), colorField.second.first.StartOffset); //TODO Get root parameter index somehow
}

void QuantumEngine::Rendering::DX12::HLSLMaterial::SetColor(const std::string& fieldName, const Color& color)
{
    auto field = m_colorValues.find(fieldName);

    if (field != m_colorValues.end()) {
        (*field).second.second = color;
    }
}
