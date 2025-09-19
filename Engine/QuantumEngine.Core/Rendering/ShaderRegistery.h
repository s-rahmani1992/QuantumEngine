#pragma once
#include "../BasicTypes.h"
#include <string>

namespace QuantumEngine::Rendering {
	class ShaderProgram;
	class Shader;

	class ShaderRegistery {
	public:
		virtual void RegisterShaderProgram(const std::string& name, const ref<ShaderProgram>& program, bool isRT = false) = 0;
		virtual ref<ShaderProgram> CreateAndRegisterShaderProgram(const std::string& name, const std::initializer_list<ref<Shader>>& shaders, bool isRT = false) = 0;
	};
}
