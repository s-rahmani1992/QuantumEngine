#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <string>

namespace QuantumEngine::Rendering::DX12::RayTracing {
	class HLSLRayTracingProgram;

	struct HLSLRayTracingProgramProperties {
		std::string shaderModel;
		std::string rayGenerationFunction;
		std::string intersectionFunction;
		std::string anyHitFunction;
		std::string closestHitFunction;
		std::string missFunction;
	};

	class HLSLRayTracingProgramImporter {
	public:
		/// <summary>
		/// creates ray tracing shader program from file
		/// </summary>
		/// <param name="hlslFile">file's address</param>
		/// <param name="properties">import description</param>
		/// <param name="error">if creation fails, contains error description</param>
		/// <returns></returns>
		static ref<HLSLRayTracingProgram> ImportShader(const std::wstring& hlslFile, const HLSLRayTracingProgramProperties& properties, std::string& error);
	};
}