#pragma once
#include "pch.h"
#include "Rendering/GPUDeviceManager.h"

using namespace Microsoft::WRL;

namespace QuantumEngine {
	namespace Rendering {
		namespace DX12 {
			class DX12GPUDeviceManager : public GPUDeviceManager
			{
			public:
				virtual bool Initialize() override;
				~DX12GPUDeviceManager();
			private:
				ComPtr<IDXGIFactory7> m_factory;
				ComPtr<IDXGIAdapter4> m_adapter;
				ComPtr<ID3D12Device10> m_device;

#ifdef _DEBUG
				Microsoft::WRL::ComPtr<ID3D12Debug6> m_d3d12_dubug;
				Microsoft::WRL::ComPtr<IDXGIDebug1> m_dxgi_debug;
#endif
			};
		}
	}
	
}
