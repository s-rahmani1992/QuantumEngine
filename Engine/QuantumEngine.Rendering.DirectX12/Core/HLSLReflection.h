#pragma once

#include "pch.h"
#include "BasicTypes.h"
#include <string>
#include <vector>

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	struct RootConstantVariableData {
		std::string name;
		D3D12_SHADER_VARIABLE_DESC variableDesc;
	};

	struct ResourceVariableData {
		UInt32 rootParameterIndex;
		std::string name;
		D3D12_SHADER_INPUT_BIND_DESC resourceData;
	};

	struct RootConstantBufferData {
		UInt32 rootParameterIndex;
		std::string name;
		D3D12_SHADER_INPUT_BIND_DESC resourceData;
		D3D12_ROOT_CONSTANTS registerData;
		std::vector<RootConstantVariableData> rootConstants;
	};

	class HLSLReflection {
	public:
		HLSLReflection() = default;

		/// <summary>
		/// Merges the ray reflection data from a shader into this reflection data
		/// </summary>
		/// <param name="shaderReflection">Raw DX12 Reflection</param>
		void AddShaderReflection(ID3D12ShaderReflection* shaderReflection);

		/// <summary>
		/// Creates the DirectX 12 root signature from this reflection data
		/// </summary>
		/// <param name="device">DX12 Device for creating root signature</param>
		/// <param name="flag">root signature flag</param>
		/// <param name="error">output string containing error if creation fails</param>
		/// <returns></returns>
		ComPtr<ID3D12RootSignature> CreateRootSignature(const ComPtr<ID3D12Device10>& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::string& error);
		
		/// <summary>
		/// Gets list of root constant data
		/// </summary>
		/// <returns></returns>
		inline std::vector<RootConstantBufferData>& GetRootConstants() { return m_rootConstants; }
		
		/// <summary>
		/// Gets list of resource variable data
		/// </summary>
		/// <returns></returns>
		inline std::vector<ResourceVariableData>& GetResourceVariables() { return m_resourceVariables; }
		
		/// <summary>
		/// Gets list of sampler variable data
		/// </summary>
		/// <returns></returns>
		inline std::vector<ResourceVariableData>& GetSamplerVariables() { return m_samplerVariables; }
	
	private:
		std::vector<RootConstantBufferData> m_rootConstants;
		std::vector<ResourceVariableData> m_resourceVariables;
		std::vector<ResourceVariableData> m_samplerVariables;

		UInt32 m_rootparameterCount = 0;
	};
}