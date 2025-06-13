#include "pch.h"
#include "DX12MeshController.h"
#include "Core/Mesh.h"

QuantumEngine::Rendering::DX12::DX12MeshController::DX12MeshController(const ref<Mesh>& mesh)
	:m_mesh(mesh)
{
}

QuantumEngine::Rendering::DX12::DX12MeshController::~DX12MeshController()
{
	delete[] m_layoutDesc.pInputElementDescs;
}

bool QuantumEngine::Rendering::DX12::DX12MeshController::Initialize(const ComPtr<ID3D12Device10>& device)
{
	//Create upload and vertex buffers
	D3D12_HEAP_PROPERTIES uploadHeapProps
	{
	.Type = D3D12_HEAP_TYPE_UPLOAD,
	.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	.CreationNodeMask = 0,
	.VisibleNodeMask = 0,
	};

	D3D12_RESOURCE_DESC bufferDesc
	{
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	bufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	bufferDesc.Width = sizeof(Vertex) * m_mesh->GetVertexCount(),
	bufferDesc.Height = 1,
	bufferDesc.DepthOrArraySize = 1,
	bufferDesc.MipLevels = 1,
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN,
	bufferDesc.SampleDesc.Count = 1,
	bufferDesc.SampleDesc.Quality = 0,
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	D3D12_HEAP_PROPERTIES bufferHeapProps{};
	bufferHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	bufferHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	bufferHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	bufferHeapProps.CreationNodeMask = 0;
	bufferHeapProps.VisibleNodeMask = 0;

	if(FAILED(device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_uploadVertexBuffer))))
		return false;

	if(FAILED(device->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer))))
		return false;

	//Create upload and index buffers
	
	D3D12_RESOURCE_DESC indexDesc
	{
	indexDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	indexDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
	indexDesc.Width = sizeof(UInt32) * m_mesh->GetIndexCount(),
	indexDesc.Height = 1,
	indexDesc.DepthOrArraySize = 1,
	indexDesc.MipLevels = 1,
	indexDesc.Format = DXGI_FORMAT_UNKNOWN,
	indexDesc.SampleDesc.Count = 1,
	indexDesc.SampleDesc.Quality = 0,
	indexDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	indexDesc.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	if (FAILED(device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &indexDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_uploadIndexBuffer))))
		return false;

	if (FAILED(device->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE, &indexDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer))))
		return false;

	//Input layout
	D3D12_INPUT_ELEMENT_DESC* elements = new D3D12_INPUT_ELEMENT_DESC[3];
	elements[0] = { "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	elements[1] = { "uv", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	elements[2] = { "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	m_layoutDesc.NumElements = 3;
	m_layoutDesc.pInputElementDescs = elements;

	// Buffer views
	m_bufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_bufferView.SizeInBytes = sizeof(Vertex) * m_mesh->GetVertexCount();
	m_bufferView.StrideInBytes = sizeof(Vertex);

	// Index view
	m_indexView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexView.SizeInBytes = sizeof(UInt32) * m_mesh->GetIndexCount();
	m_indexView.Format = DXGI_FORMAT_R32_UINT;
}

void QuantumEngine::Rendering::DX12::DX12MeshController::UploadToGPU(ComPtr<ID3D12GraphicsCommandList7>& uploadCommandList)
{
	auto meshData = m_mesh->GetVertexData();
	auto size = sizeof(Vertex) * m_mesh->GetVertexCount();
	void* uploadAddress;
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = size - 1;
	m_uploadVertexBuffer->Map(0, &range, &uploadAddress);
	std::memcpy(uploadAddress, meshData, range.End);
	m_uploadVertexBuffer->Unmap(0, &range);
	uploadCommandList->CopyBufferRegion(m_vertexBuffer.Get(), 0, m_uploadVertexBuffer.Get(), 0, size);

	auto indexData = m_mesh->GetIndexData();
	size = sizeof(UInt32) * m_mesh->GetIndexCount();
	range.Begin = 0;
	range.End = size - 1;
	m_uploadIndexBuffer->Map(0, &range, &uploadAddress);
	std::memcpy(uploadAddress, indexData, range.End);
	m_uploadIndexBuffer->Unmap(0, &range);
	uploadCommandList->CopyBufferRegion(m_indexBuffer.Get(), 0, m_uploadIndexBuffer.Get(), 0, size);
}
