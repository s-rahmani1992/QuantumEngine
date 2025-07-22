#pragma once
#include "pch.h"

namespace QuantumEngine::Rendering::DX12 {
	class DescriptorUtilities
	{
	public:
		static const D3D12_HEAP_PROPERTIES CommonUploadHeapProps;
		static const D3D12_HEAP_PROPERTIES CommonDefaultHeapProps;
	};
}
