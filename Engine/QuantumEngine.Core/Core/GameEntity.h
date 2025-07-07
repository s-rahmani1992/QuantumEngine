#pragma once
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;
	class Transform;

	namespace Rendering {
		class Material;
	}
}

namespace QuantumEngine {
	
	class GameEntity {
	public:
		GameEntity(const ref<Transform>& transform, const ref<Mesh>& mesh, const ref<Rendering::Material>& material)
			:m_transform(transform), m_mesh(mesh), m_material(material) { }

		inline ref<Mesh> GetMesh() const { return m_mesh; }
		inline ref<Rendering::Material> GetMaterial() { return m_material; }
		inline ref<Transform> GetTransform() const { return m_transform; }
	private:
		ref<Mesh> m_mesh;
		ref<Rendering::Material> m_material;
		ref<Transform> m_transform;
	};
}