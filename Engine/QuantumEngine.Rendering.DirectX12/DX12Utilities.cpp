#include "pch.h"
#include "DX12Utilities.h"

namespace DirectX12 = QuantumEngine::Rendering::DX12;

const D3D12_HEAP_PROPERTIES DirectX12::DescriptorUtilities::CommonUploadHeapProps = D3D12_HEAP_PROPERTIES{
	.Type = D3D12_HEAP_TYPE_UPLOAD,
	.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	.CreationNodeMask = 0,
	.VisibleNodeMask = 0, 
};

const D3D12_HEAP_PROPERTIES DirectX12::DescriptorUtilities::CommonDefaultHeapProps = D3D12_HEAP_PROPERTIES{
	.Type = D3D12_HEAP_TYPE_DEFAULT,
	.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	.CreationNodeMask = 0,
	.VisibleNodeMask = 0,
};
