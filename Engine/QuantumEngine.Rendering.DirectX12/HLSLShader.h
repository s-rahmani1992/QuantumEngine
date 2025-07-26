#pragma once
#include "Rendering/Shader.h"
#include <vector>
#include <map>
#include <string>

#define RAYGEN_NAME L"rayGen"
#define MISS_NAME L"miss"
#define CLOSEHIT_NAME L"chs"
#define INTERSECTION_NAME L"intersect"
#define ANYHIT_NAME L"anyhit"

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	enum DX12_Shader_Type { //TODO If possible, convert to enum class
		VERTEX_SHADER = 0,
		PIXEL_SHADER = 1,
		LIB_SHADER = 2,
	};

	struct HLSLRootConstantData {
		std::string name;
		D3D12_SHADER_VARIABLE_DESC variableDesc;
		D3D12_ROOT_CONSTANTS registerData;
	};

	struct HLSLShaderReflection {
		std::vector<HLSLRootConstantData> rootConstants;
		std::vector<std::pair<std::string, D3D12_SHADER_INPUT_BIND_DESC>> boundVariables;
	};

	class HLSLShader : public Shader {
	public:
		HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType);
		HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12ShaderReflection>& shaderReflection);
		HLSLShader(Byte* byteCode, UInt64 codeLength, DX12_Shader_Type shaderType, ComPtr<ID3D12LibraryReflection>& libraryReflection);
		inline HLSLShaderReflection* GetReflection() { return &m_reflection; }
		inline D3D12_DXIL_LIBRARY_DESC* GetDXIL() { return &m_dxilData; }
		D3D12_HIT_GROUP_DESC GetHitGroup(const std::wstring& exportName);
		std::wstring& GetRayGenExport() { return m_entryPoints[RAYGEN_NAME]; }
		std::wstring& GetMissExport() { return m_entryPoints[MISS_NAME]; }
	private:
		void FillReflection(ComPtr<ID3D12ShaderReflection>& shaderReflection);
		void FillReflection(ComPtr<ID3D12LibraryReflection>& libraryReflection);
		void InitializeDXIL();
		static UInt32 m_shaderCounter;
	private:
		DX12_Shader_Type m_shaderType;
		HLSLShaderReflection m_reflection;
		std::map<std::wstring, std::wstring> m_entryPoints;
		D3D12_DXIL_LIBRARY_DESC m_dxilData;
		std::vector<D3D12_EXPORT_DESC> m_exportDescs;
	};
}