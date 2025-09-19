#pragma once
#include "pch.h"
#include "Rendering/ShaderRegistery.h"
#include <map>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	class HLSLShaderProgram;

	class DX12ShaderRegistery : public ShaderRegistery
	{
	public:
		void Initialize(const ComPtr<ID3D12Device10>& device);
		ref<HLSLShaderProgram> GetShaderProgram(const std::string& name);
		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) override;
		virtual ref<ShaderProgram> CreateAndRegisterShaderProgram(const std::string& name, const std::initializer_list<ref<Shader>>& shaders, bool isRT = false) override;
	private:
		ComPtr<ID3D12Device10> m_device;
		std::map<std::string, ref<HLSLShaderProgram>> m_shaders;
	};
}


