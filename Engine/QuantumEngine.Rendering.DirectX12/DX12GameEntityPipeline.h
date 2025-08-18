#pragma once
#include "pch.h"
#include <vector>
#include <memory>
#include "BasicTypes.h"

namespace QuantumEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;
	struct DX12EntityGPUData;
	class HLSLMaterial;
	class DX12MeshController;

	class DX12GameEntityPipeline
	{
	public:
		bool Initialize(const ComPtr<ID3D12Device10>& device, const DX12EntityGPUData& entityGPUData, DXGI_FORMAT depthFormat);
		void Render(ComPtr<ID3D12GraphicsCommandList7>& commandList);

	private:
		ref<HLSLMaterial> m_material;
		ComPtr<ID3D12DescriptorHeap> m_transformHeap;
		ref<DX12MeshController> m_meshController;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipeline;
	};
}
