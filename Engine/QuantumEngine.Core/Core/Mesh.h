#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include <vector>

namespace QuantumEngine {
	struct Vertex {
		Vector3 position;
		Vector2 uv;
		Vector3 normal;

		Vertex() : Vertex(Vector3(0.0f), Vector2(0.0f), Vector3(0.0f))
		{
		}

		Vertex(const Vector3& pos, const Vector2& tex, const Vector3& normal)
			:position(pos), uv(tex), normal(normal)
		{
		}
	};

	class Mesh {
	private:
		UInt32 m_vertexCount;
		std::vector<Vertex> m_vertices;
		std::vector<UInt32> m_indices;
	public:
		UInt32 GetVertexCount() const { return m_vertexCount; }
		UInt32 GetIndexCount() const { return m_indices.size(); }
		Vertex* GetVertexData() { return &m_vertices[0]; }
		UInt32* GetIndexData() { return &m_indices[0]; }
		Mesh(const std::vector<Vertex>& vertices, const std::vector<UInt32>& indices);
	};
}