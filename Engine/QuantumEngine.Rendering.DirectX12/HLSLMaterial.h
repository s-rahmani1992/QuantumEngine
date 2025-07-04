#pragma once
#include "pch.h"
#include "Rendering/Material.h"
#include <map>
#include <vector>

using namespace Microsoft::WRL;

namespace QuantumEngine {
	struct Vector2;
	class Texture2D;
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
		void SetTexture2D(const std::string& fieldName, const ref<Texture2D>& texValue);

	private:
		template<typename T>
		struct RootConstantData {
			UInt32 rootParamIndex;
			UInt32 offset;
			UInt32 size;
			T value;
		};

		struct SRVData {
			UInt32 rootParamIndex;
			ComPtr<ID3D12DescriptorHeap> gpuHandle;
		};

		std::map<std::string, D3D12_SHADER_VARIABLE_DESC> ExtractConstantBuffers(const ComPtr<ID3D12ShaderReflection>& reflector);
		void UpdateHeaps();
	private:
		std::map<std::string, RootConstantData<Float>> m_floatValues;
		std::map<std::string, RootConstantData<Vector2>> m_vector2Values;
		std::map<std::string, RootConstantData<Color>> m_colorValues;
		std::map<std::string, SRVData> m_texture2DValues;
		std::vector<ID3D12DescriptorHeap*> m_allHeaps;
	};
}