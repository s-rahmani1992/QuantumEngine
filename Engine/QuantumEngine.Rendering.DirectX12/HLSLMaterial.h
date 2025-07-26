#pragma once
#include "pch.h"
#include "Rendering/Material.h"
#include <map>
#include <vector>

using namespace Microsoft::WRL;

namespace QuantumEngine {
	struct Vector2;
	struct Vector3;
	struct Matrix4;
	class Texture2D;
}

namespace QuantumEngine::Rendering::DX12 {
	class HLSLMaterial : public Material {
	public:
		HLSLMaterial(const ref<ShaderProgram>& program);
		~HLSLMaterial();
		bool Initialize();
		void RegisterValues(ComPtr<ID3D12GraphicsCommandList7>& commandList);
		void RegisterComputeValues(ComPtr<ID3D12GraphicsCommandList7>& commandList);
		void SetColor(const std::string& fieldName, const Color& color);
		void SetFloat(const std::string& fieldName, const Float& fValue);
		void SetVector2(const std::string& fieldName, const Vector2& fValue);
		void SetVector3(const std::string& fieldName, const Vector3& fValue);
		void SetMatrix(const std::string& fieldName, const Matrix4& matrixValue);
		void SetTexture2D(const std::string& fieldName, const ref<Texture2D>& texValue);
		void SetDescriptorHeap(const std::string& fieldName, const ComPtr<ID3D12DescriptorHeap>& descriptorHeap);
	private:
		struct constantBufferData {
		UInt32 rootParamIndex;
		Byte* location;
		UInt32 size;
		};

		struct HeapData {
			UInt32 rootParamIndex;
			ComPtr<ID3D12DescriptorHeap> descriptor;
			D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle;
		};

		std::map<std::string, D3D12_SHADER_VARIABLE_DESC> ExtractConstantBuffers(const ComPtr<ID3D12ShaderReflection>& reflector);
		void SetRootConstant(const std::string& fieldName, void* data, UInt32 size);
	private:
		std::map<std::string, HeapData> m_heapValues;
		std::map<std::string, Byte*> m_rootConstantVariableLocations;
		std::vector<constantBufferData> m_constantRegisterValues;
		Byte* m_variableData;
	};
}