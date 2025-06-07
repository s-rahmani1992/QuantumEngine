#pragma once

#include "Rendering/GraphicContext.h"
#include "BasicTypes.h"
#include <vector>

namespace QuantumEngine {
	namespace Platform {
		class GraphicWindow;
	}
}

namespace QuantumEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;
	class DX12CommandExecuter;

	class DX12GraphicContext : public GraphicContext
	{
	public:
		DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window);
		bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory);
		virtual void Render() override;
	private:
		UInt8 m_bufferCount;
		ComPtr<IDXGISwapChain4> m_swapChain;
		std::vector<ComPtr<ID3D12Resource2>> m_buffers;
		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;

		ref<DX12CommandExecuter> m_commandExecuter;
		ref<QuantumEngine::Platform::GraphicWindow> m_window;

		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12GraphicsCommandList7> m_commandList;
	};
}
