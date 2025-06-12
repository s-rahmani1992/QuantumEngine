#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include "Rendering/GPUDeviceManager.h"
#include <map>

using namespace Microsoft::WRL;

namespace QuantumEngine {
	class Mesh;

	namespace Platform {
		class GraphicWindow;
	}

	namespace Rendering {
		class GraphicContext;
	}
}

namespace QuantumEngine::Rendering::DX12 {
	class DX12CommandExecuter;
	class DX12MeshController;

	class DX12GPUDeviceManager : public GPUDeviceManager
	{
	public:
		virtual bool Initialize() override;
		virtual ref<GraphicContext> CreateContextForWindows(ref<QuantumEngine::Platform::GraphicWindow>& window) override;
		virtual void UploadMeshToGPU(const ref<Mesh>& mesh) override;
		~DX12GPUDeviceManager();
		ref<DX12CommandExecuter> CreateCommandExecuter();
	private:
		ref<DX12CommandExecuter> CreateUploadCommandExecuter();

		ComPtr<IDXGIFactory7> m_factory;
		ComPtr<IDXGIAdapter4> m_adapter;
		ComPtr<ID3D12Device10> m_device;

		ComPtr<ID3D12CommandAllocator> m_uploadCommandAllocator;
		ComPtr<ID3D12GraphicsCommandList7> m_uploadCommandList;
		ref<DX12CommandExecuter> m_meshUploadCommandExecuter;
		std::map<ref<Mesh>, ref<DX12MeshController>> m_meshes;

#ifdef _DEBUG
		Microsoft::WRL::ComPtr<ID3D12Debug6> m_d3d12_dubug;
		Microsoft::WRL::ComPtr<IDXGIDebug1> m_dxgi_debug;
#endif
	};
}
