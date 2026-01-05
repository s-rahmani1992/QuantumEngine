#pragma once
#include "BasicTypes.h"
#include <string>

namespace QuantumEngine::Rendering::DX12::Shader {
	class HLSLRasterizationProgram;

	struct HLSLRasterizationProgramImportDesc {
		std::string shaderModel; //TODO : Use enum class if possible
		std::string vertexMainFunction;
		std::string pixelMainFunction;
		std::string geometryMainFunction;
	};

	/// <summary>
	/// Class responsible for compiling hlsl files into rasterization shader programs
	/// </summary>
	class HLSLRasterizationProgramImporter {
	public:
		/// <summary>
		/// Creates a rasterization shader program from an hlsl file
		/// </summary>
		/// <param name="hlslFile">path of the hlsl file</param>
		/// <param name="properties">import properties</param>
		/// <param name="error">if import not successful, contains error message as output</param>
		/// <returns></returns>
		static ref<HLSLRasterizationProgram> ImportShader(const std::wstring& hlslFile, const HLSLRasterizationProgramImportDesc& properties, std::string& error);
	};
}