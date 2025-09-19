#include "pch.h"
#include "DX12ShaderRegistery.h"
#include "HLSLShaderProgram.h"

void QuantumEngine::Rendering::DX12::DX12ShaderRegistery::Initialize(const ComPtr<ID3D12Device10>& device)
{
	m_device = device;
}

ref<QuantumEngine::Rendering::DX12::HLSLShaderProgram> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::GetShaderProgram(const std::string& name)
{
	auto it = m_shaders.find(name);
	if (it != m_shaders.end())
		return (*it).second;
	return nullptr;
}

void QuantumEngine::Rendering::DX12::DX12ShaderRegistery::RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT)
{
	ref<HLSLShaderProgram> hlslProgram = std::dynamic_pointer_cast<HLSLShaderProgram>(program);

	if (hlslProgram->Initialize(m_device, isRT))
		m_shaders[name] = std::dynamic_pointer_cast<HLSLShaderProgram>(hlslProgram);
}

ref<QuantumEngine::Rendering::ShaderProgram> QuantumEngine::Rendering::DX12::DX12ShaderRegistery::CreateAndRegisterShaderProgram(const std::string& name, const std::initializer_list<ref<Shader>>& shaders, bool isRT)
{
	ref<HLSLShaderProgram> program = std::make_shared<HLSLShaderProgram>(shaders);

	if (program->Initialize(m_device, isRT))
		m_shaders[name] = program;

	return program;
}
