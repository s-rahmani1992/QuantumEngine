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
		GameEntity(const ref<Transform>& transform, const ref<Mesh>& mesh
			, const ref<Rendering::Material>& material, const ref<Rendering::Material>& rtMaterial = nullptr)
			:m_transform(transform), m_mesh(mesh), m_material(material), m_rtMaterial(rtMaterial) { }

		inline ref<Mesh> GetMesh() const { return m_mesh; }
		inline ref<Rendering::Material> GetMaterial() { return m_material; }
		inline ref<Rendering::Material> GetRTMaterial() { return m_rtMaterial; }
		inline ref<Transform> GetTransform() const { return m_transform; }
	private:
		ref<Mesh> m_mesh;
		ref<Rendering::Material> m_material;
		ref<Transform> m_transform;
		ref<Rendering::Material> m_rtMaterial;
	};
}