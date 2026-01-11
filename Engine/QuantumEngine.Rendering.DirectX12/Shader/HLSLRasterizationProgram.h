#pragma once
#include "pch.h"
#include "Rendering/ShaderProgram.h"
#include <string>
#include "../Core/HLSLReflection.h"

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	class HLSLShader;
}

namespace QuantumEngine::Rendering::DX12::Shader {
	class HLSLRasterizationProgram : public ShaderProgram {
	public:
		HLSLRasterizationProgram(const std::vector<ref<HLSLShader>>& shaders);
		virtual ~HLSLRasterizationProgram() = default;

		/// <summary>
		/// Creates the DirectX 12 root signature for this shader program
		/// </summary>
		/// <param name="device"></param>
		/// <returns></returns>
		bool InitializeRootSignature(const ComPtr<ID3D12Device10>& device);

		/// <summary>
		/// Gets the parameter layout for this shader program
		/// </summary>
		/// <returns></returns>
		inline HLSLReflection* GetReflectionData() { return &m_reflection; }

		/// <summary>
		/// Gets the root signature for this shader program
		/// </summary>
		/// <returns></returns>
		inline ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_rootSignature; }

		/// <summary>
		/// Gets the vertex shader if exists
		/// </summary>
		/// <returns></returns>
		inline ref<HLSLShader> GetVertexShader() const { return m_vertexShader; }

		/// <summary>
		/// Gets the pixel shader if exists
		/// </summary>
		/// <returns></returns>
		inline ref<HLSLShader> GetPixelShader() const { return m_pixelShader; }

		/// <summary>
		/// Gets the geometry shader if exists
		/// </summary>
		/// <returns></returns>
		inline ref<HLSLShader> GetGeometryShader() const { return m_geometryShader; }

	private:
		ref<HLSLShader> m_vertexShader;
		ref<HLSLShader> m_pixelShader;
		ref<HLSLShader> m_geometryShader;
		
		HLSLReflection m_reflection;
		ComPtr<ID3D12RootSignature> m_rootSignature;
	};
}