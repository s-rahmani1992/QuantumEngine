#pragma once
#include "BasicTypes.h"

using namespace Microsoft::WRL;

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering::DX12 {
	class DX12MeshController
	{
	public:
		DX12MeshController(const ref<Mesh>& mesh);
		~DX12MeshController();
		inline D3D12_INPUT_LAYOUT_DESC* GetLayoutDesc() { return &m_layoutDesc; }
		inline D3D12_VERTEX_BUFFER_VIEW* GetVertexView() { return &m_bufferView; }
		inline D3D12_INDEX_BUFFER_VIEW* GetIndexView() { return &m_indexView; }
		inline ref<Mesh> GetMesh() { return m_mesh; }
		bool Initialize(const ComPtr<ID3D12Device10>& device);
		void UploadToGPU(ComPtr<ID3D12GraphicsCommandList7>& uploadCommandList);
		ComPtr<ID3D12Resource2> GetBLAS() { return m_bottomLevelAccelationData; }
	private:
		ref<Mesh> m_mesh;
		ComPtr<ID3D12Resource2> m_uploadVertexBuffer;
		ComPtr<ID3D12Resource2> m_vertexBuffer;
		ComPtr<ID3D12Resource2> m_uploadIndexBuffer;
		ComPtr<ID3D12Resource2> m_indexBuffer;
		ComPtr<ID3D12Resource2> m_rtScratchBuffer;
		ComPtr<ID3D12Resource2> m_bottomLevelAccelationData;
		D3D12_INPUT_LAYOUT_DESC m_layoutDesc;
		D3D12_VERTEX_BUFFER_VIEW m_bufferView;
		D3D12_INDEX_BUFFER_VIEW m_indexView;
	};
}
