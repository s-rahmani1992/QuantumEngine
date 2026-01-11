#pragma once
#include "pch.h"
#include "Rendering/ShaderProgram.h"
#include "../Core/HLSLReflection.h"

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12::Compute {
	class HLSLComputeProgram : public ShaderProgram
	{
	public:
		HLSLComputeProgram(Byte* byteCode, UInt64 codeLength, ComPtr<ID3D12ShaderReflection>& shaderReflection);
		
		/// <summary>
		/// Creates the root signature for this shader program
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
		/// Gets pointer to byte code
		/// </summary>
		/// <returns></returns>
		inline Byte* GetByteCode() const { return m_byteCode; }

		/// <summary>
		/// Gets length of byte code
		/// </summary>
		/// <returns></returns>
		inline UInt64 GetCodeLength() const { return m_codeLength; }

	private:
		Byte* m_byteCode;
		UInt64 m_codeLength;

		HLSLReflection m_reflection;
		ComPtr<ID3D12RootSignature> m_rootSignature;
	};
}