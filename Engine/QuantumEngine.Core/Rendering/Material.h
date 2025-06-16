#pragma once
#include <string>
#include <map>
#include "../Core/Color.h"
#include "../BasicTypes.h"

namespace QuantumEngine {
	namespace Rendering {
		class ShaderProgram;
	}
}

namespace QuantumEngine::Rendering {
	class Material {
	public:
		Material(const ref<ShaderProgram>& program) 
			:m_program(program)
		{

		}
		virtual ~Material() = default;
		ref<ShaderProgram> GetProgram() { return m_program; }
	protected:
		ref<ShaderProgram> m_program;
	};
}