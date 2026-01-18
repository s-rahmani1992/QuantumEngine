#pragma once

#include "pch.h"
#include "BasicTypes.h"
#include <string>
#include <vector>
#include <Core/Matrix4.h>
#include <Rendering/Material.h>

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
		/// Merges the reflection data from the shader into this reflection data
		/// </summary>
		/// <param name="shaderReflection">Raw DX12 Reflection</param>
		void AddShaderReflection(ID3D12ShaderReflection* shaderReflection);

		/// <summary>
		/// Merges the reflection data from the ray tracing function into this reflection data
		/// </summary>
		/// <param name="functionReflection"></param>
		void AddShaderReflection(ID3D12FunctionReflection* functionReflection);

		/// <summary>
		/// Creates the DirectX 12 root signature from this reflection data
		/// </summary>
		/// <param name="device">DX12 Device for creating root signature</param>
		/// <param name="flag">root signature flag</param>
		/// <param name="error">output string containing error if creation fails</param>
		/// <returns></returns>
		ComPtr<ID3D12RootSignature> CreateRootSignature(const ComPtr<ID3D12Device10>& device, D3D12_ROOT_SIGNATURE_FLAGS flag, std::string& error);
		
		/// <summary>
		/// Creates Reflection for creating User material
		/// </summary>
		/// <param name="rtField">determine if the destination is for ray tracing material</param>
		/// <returns></returns>
		MaterialReflection CreateMaterialReflection(bool rtField);

		/// <summary>
		/// Gets index of root parameter
		/// </summary>
		/// <param name="name">name of the field</param>
		/// <returns>returns -1 if the parameter not found</returns>
		UInt32 GetRootParameterIndexByName(const std::string& name) const;

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
	
		/// <summary>
		/// Gets total number of root parameters
		/// </summary>
		/// <returns></returns>
		inline UInt32 GetRootParameterCount() const { return m_rootparameterCount; }

		/// <summary>
		/// Gets pointer to Resource Element by root parameter index
		/// </summary>
		/// <param name="rootParameterIndex"></param>
		/// <returns></returns>
		ResourceVariableData* GetResourceVariableByRootIndex(UInt32 rootParameterIndex);

		/// <summary>
		/// Gets pointer to the root constant element by root parameter index
		/// </summary>
		/// <param name="rootParameterIndex"></param>
		/// <returns></returns>
		RootConstantBufferData* GetRootConstantByRootIndex(UInt32 rootParameterIndex);

		/// <summary>
		/// Gets the sum of the parameters' sizes in bytes
		/// </summary>
		/// <returns></returns>
		UInt32 GetTotalVariableSize();

	private:

		template<typename T>
		void AddShaderInputBindings(const D3D12_SHADER_INPUT_BIND_DESC& resourceBinding, T* reflection);

	private:
		std::vector<RootConstantBufferData> m_rootConstants;
		std::vector<ResourceVariableData> m_resourceVariables;
		std::vector<ResourceVariableData> m_samplerVariables;

		UInt32 m_rootparameterCount = 0;
	};

	template<typename T>
	inline void HLSLReflection::AddShaderInputBindings(const D3D12_SHADER_INPUT_BIND_DESC& boundResource, T* reflection)
	{
		std::string name(boundResource.Name);

		// Constant Buffer is processed separately. Because it can be either a single buffer or collection of root constants.
		if (boundResource.Type == D3D_SIT_CBUFFER)
		{
			auto rootConstantIt = std::find_if(m_rootConstants.begin(), m_rootConstants.end(),
				[&name](const RootConstantBufferData& data) { return data.name == name; });

			if (rootConstantIt != m_rootConstants.end())
				return; //checking if the resource is already processed because it can exist in multiple shaders

			auto recourceIt = std::find_if(m_resourceVariables.begin(), m_resourceVariables.end(),
				[&name](const ResourceVariableData& data) { return data.name == name; });

			if (recourceIt != m_resourceVariables.end())
				return; //checking if the resource is already processed because it can exist in multiple shaders

			auto samplerIt = std::find_if(m_samplerVariables.begin(), m_samplerVariables.end(),
				[&name](const ResourceVariableData& data) { return data.name == name; });

			if (samplerIt != m_samplerVariables.end())
				return; //checking if the resource is already processed because it can exist in multiple shaders

			ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByName(boundResource.Name);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			cb->GetDesc(&cbDesc);

			//if constant buffer size per variable is less than Matrix4, then it is considered as a holder of root constants
			if (cbDesc.Size / cbDesc.Variables <= sizeof(Matrix4)) { //TODO Find better condition for constant buffer checking if possible
				RootConstantBufferData cData{
					.rootParameterIndex = m_rootparameterCount,
					.name = name,
					.resourceData = boundResource,
					.registerData = D3D12_ROOT_CONSTANTS{
						.ShaderRegister = boundResource.BindPoint,
						.RegisterSpace = boundResource.Space,
						.Num32BitValues = cbDesc.Size / 4,
					},
				};

				for (UINT v = 0; v < cbDesc.Variables; ++v) {
					ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(v);
					D3D12_SHADER_VARIABLE_DESC varDesc;
					var->GetDesc(&varDesc);

					cData.rootConstants.push_back(RootConstantVariableData{
						.name = std::string(varDesc.Name),
						.variableDesc = varDesc,
						});
				}

				m_rootConstants.push_back(cData);
			}
			else {
				m_resourceVariables.push_back(ResourceVariableData{
					.rootParameterIndex = m_rootparameterCount,
					.name = name,
					.resourceData = boundResource });
			}

			m_rootparameterCount++;
		}
		// Sampler is considered as separate type because it can either contribute as root parameter or as static sampler
		else if (boundResource.Type == D3D_SIT_SAMPLER) {
			m_samplerVariables.push_back(ResourceVariableData{ .rootParameterIndex = 9999, .name = name, .resourceData = boundResource });
		}
		else {
			m_resourceVariables.push_back(ResourceVariableData{ .rootParameterIndex = m_rootparameterCount, .name = name, .resourceData = boundResource });
			m_rootparameterCount++;
		}
	}
}