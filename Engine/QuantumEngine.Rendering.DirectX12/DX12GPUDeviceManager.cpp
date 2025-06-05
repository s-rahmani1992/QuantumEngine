#include "pch.h"
#include "DX12GPUDeviceManager.h"

bool QuantumEngine::Rendering::DX12::DX12GPUDeviceManager::Initialize()
{
	// Create Debug layer in Debug build
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_d3d12_dubug)))) {
		m_d3d12_dubug->EnableDebugLayer();

		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_debug)))) {
			m_dxgi_debug->EnableLeakTrackingForThread();
		}
	}
#endif

	//Find High Performance, DirectX 12 compatible GPU
	if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory)))) {
		return false;
	}

	bool adapterFound = false;

	//Loop through available adapters
	for (int adapterId = 0; m_factory->EnumAdapterByGpuPreference(adapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)) != DXGI_ERROR_NOT_FOUND; adapterId++) {
		DXGI_ADAPTER_DESC3 desc;
		m_adapter->GetDesc3(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // Don't select the Basic Render Driver adapter.
			continue;

		// Checking for Directx 12 capability and creating device at the same time.
		if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device)))) {
			adapterFound = true;
			break;
		}
	}

	if (!adapterFound)
		return false;
}

QuantumEngine::Rendering::DX12::DX12GPUDeviceManager::~DX12GPUDeviceManager()
{
#ifdef _DEBUG
	if (m_dxgi_debug) {
		OutputDebugStringW(L"Live Object Reports:\n");
		m_dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	}
#endif
}
