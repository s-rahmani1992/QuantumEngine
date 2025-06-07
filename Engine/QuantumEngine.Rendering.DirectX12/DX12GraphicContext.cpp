#include "pch.h"
#include "DX12GraphicContext.h"
#include "DX12CommandExecuter.h"
#include "Platform/GraphicWindow.h"


bool QuantumEngine::Rendering::DX12::DX12GraphicContext::Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory)
{
	//command allocator
	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator))))
		return false;

	//Command List
	if (FAILED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_commandList))))
		return false;

	//Create Swap Chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc1
	{
	.Width = m_window->GetWidth(),
	.Height = m_window->GetHeight(),
	.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	.Stereo = false,
	.SampleDesc = DXGI_SAMPLE_DESC
		{
		.Count = 1,
		.Quality = 0,
		},
	.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
	.BufferCount = m_bufferCount,
	.Scaling = DXGI_SCALING_STRETCH,
	.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
	.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc{ 0 };
	swapChainFullScreenDesc.Windowed = true;

	ComPtr<IDXGISwapChain1> swapChain1;

	if (FAILED(factory->CreateSwapChainForHwnd(m_commandExecuter->GetQueue().Get(), m_window->GetHandle(), &swapChainDesc1, &swapChainFullScreenDesc, nullptr, &swapChain1))) {
		return false;
	}

	if (FAILED(swapChain1.As(&m_swapChain)))
		return false;

	// Create Heap for Render Target view
	D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc{
	.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	.NumDescriptors = m_bufferCount,
	.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	.NodeMask = 0,
	};
	if (FAILED(device->CreateDescriptorHeap(&rtv_desc_heap_desc, IID_PPV_ARGS(&m_rtvDescriptorHeap))))
		return false;

	auto firstHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (size_t i = 0; i < m_bufferCount; i++) {
		m_rtvHandles[i] = firstHandle;
		m_rtvHandles[i].ptr += i * handleIncrementSize;
	}

	//create buffers and render target views
	for (size_t i = 0; i < m_bufferCount; i++) {
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i]));
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = D3D12_TEX2D_RTV
			{
				.MipSlice = 0,
				.PlaneSlice = 0
			},
		};
		device->CreateRenderTargetView(m_buffers[i].Get(), &rtv_desc, m_rtvHandles[i]);
	}

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GraphicContext::Render()
{
	// Reset Commands
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	//Set Render Target
	auto m_current_buffer_index = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER beginBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_buffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
		.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},	
	};
	m_commandList->ResourceBarrier(1, &beginBarrier);
	float clearColor[] = { 0.1f, 0.7f, 0.3f, 1.0f };
	m_commandList->ClearRenderTargetView(m_rtvHandles[m_current_buffer_index], clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &m_rtvHandles[m_current_buffer_index], false, nullptr);
	//draw

	//draw

	D3D12_RESOURCE_BARRIER endBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_buffers[m_current_buffer_index].Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
		.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};
	m_commandList->ResourceBarrier(1, &endBarrier);

	m_commandExecuter->ExecuteAndWait(m_commandList.Get());
	m_swapChain->Present(1, 0);
}

QuantumEngine::Rendering::DX12::DX12GraphicContext::DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window)
	:m_bufferCount(bufferCount), m_commandExecuter(commandExecuter), m_window(window),
	 m_buffers(std::vector<ComPtr<ID3D12Resource2>>(bufferCount)),
	 m_rtvHandles(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>(bufferCount))
{
}
