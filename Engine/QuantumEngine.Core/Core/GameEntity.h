#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;

	namespace Rendering {
		class Material;
	}
}

namespace QuantumEngine {
	
	class GameEntity {
	public:
		GameEntity(const ref<Mesh>& mesh, const ref<Rendering::Material>& material)
			:m_mesh(mesh), m_material(material) { }

		inline ref<Mesh> GetMesh() const { return m_mesh; }
		inline ref<Rendering::Material> GetMaterial() { return m_material; }
	private:
		ref<Mesh> m_mesh;
		ref<Rendering::Material> m_material;
	};
}