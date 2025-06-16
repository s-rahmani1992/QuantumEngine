#pragma once
#include "pch.h"
#include "Rendering/Material.h"
#include <map>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	class HLSLMaterial : public Material {
	public:
		HLSLMaterial(const ref<ShaderProgram>& program);
		bool Initialize();
		void RegisterValues(ComPtr<ID3D12GraphicsCommandList7>& commandList);
		void SetColor(const std::string& fieldName, const Color& color);

	private:
		std::map<std::string, D3D12_SHADER_VARIABLE_DESC> ExtractConstantBuffers(const ComPtr<ID3D12ShaderReflection>& reflector);
	
	private:
		std::map<std::string, std::pair<D3D12_SHADER_VARIABLE_DESC, Color>> m_colorValues;
	};
}