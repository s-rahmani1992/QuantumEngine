#include "Mesh.h"

QuantumEngine::Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<UInt32>& indices)
	:m_vertexCount(vertices.size())
	, m_vertices(vertices), m_indices(indices)
{

}
