#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;

	namespace Rendering {
		class ShaderProgram;
	}
}

namespace QuantumEngine {
	
	class GameEntity {
	public:
		GameEntity(const ref<Mesh>& mesh, const ref<Rendering::ShaderProgram>& shader)
			:m_mesh(mesh), m_shader(shader) { }

		inline ref<Mesh> GetMesh() const { return m_mesh; }
		inline ref<Rendering::ShaderProgram> GetShaderProgram() const { return m_shader; }
	private:
		ref<Mesh> m_mesh;
		ref<Rendering::ShaderProgram> m_shader;
	};
}