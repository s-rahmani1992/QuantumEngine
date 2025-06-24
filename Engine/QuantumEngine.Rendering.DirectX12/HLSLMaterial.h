#pragma once
#include "pch.h"
#include "Rendering/Material.h"
#include <map>

using namespace Microsoft::WRL;

namespace QuantumEngine {
	struct Vector2;
}

namespace QuantumEngine::Rendering::DX12 {
	class HLSLMaterial : public Material {
	public:
		HLSLMaterial(const ref<ShaderProgram>& program);
		bool Initialize();
		void RegisterValues(ComPtr<ID3D12GraphicsCommandList7>& commandList);
		void SetColor(const std::string& fieldName, const Color& color);
		void SetFloat(const std::string& fieldName, const Float& fValue);
		void SetVector2(const std::string& fieldName, const Vector2& fValue);

	private:
		template<typename T>
		struct RootConstantData {
			UInt32 rootParamIndex;
			UInt32 offset;
			UInt32 size;
			T value;
		};

		std::map<std::string, D3D12_SHADER_VARIABLE_DESC> ExtractConstantBuffers(const ComPtr<ID3D12ShaderReflection>& reflector);
	
	private:
		std::map<std::string, RootConstantData<Float>> m_floatValues;
		std::map<std::string, RootConstantData<Vector2>> m_vector2Values;
		std::map<std::string, RootConstantData<Color>> m_colorValues;
	};
}