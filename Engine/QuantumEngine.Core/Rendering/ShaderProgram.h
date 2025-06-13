#pragma once
#include "../BasicTypes.h"
#include <initializer_list>
#include <stdexcept>
#include <vector>

namespace QuantumEngine::Rendering {
	class Shader;

	class ShaderProgram {
	public:
		ShaderProgram(const std::initializer_list<ref<Shader>>& shaders) 
			:m_shaders(shaders)
		{

		}

		ref<Shader> GetShader(Int32 index) {
			try {
				return m_shaders.at(index);
			}
			catch (const std::out_of_range& ex) {
				return nullptr;
			}
		}
	private:
		std::vector<ref<Shader>> m_shaders;
	};
}