#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <string>

namespace QuantumEngine::Rendering::DX12::Compute {
	class HLSLComputeProgram;

	struct HLSLComputeProgramImportDesc {
		std::string shaderModel; //TODO : Use enum class if possible
		std::string mainFunction;
	};

	class HLSLComputeProgramImporter
	{
	public:

		/// <summary>
		/// Creates HLSL compute shader program from file
		/// </summary>
		/// <param name="filePath">path of the hlsl file</param>
		/// <param name="properties">import properties</param>
		/// <param name="errorStr">error output if shader compilation failed</param>
		/// <returns></returns>
		static ref<HLSLComputeProgram> Import(const std::wstring& filePath, const HLSLComputeProgramImportDesc& properties, std::string& errorStr);
	};
}