#pragma once

#include "Rendering/GraphicContext.h"
#include "BasicTypes.h"
#include <vector>
#include <map>

namespace QuantumEngine {
	class GameEntity;
	class Transform;

	namespace Platform {
		class GraphicWindow;
	}
}

namespace QuantumEngine::Rendering::DX12 {
	using namespace Microsoft::WRL;
	class DX12CommandExecuter;
	class DX12MeshController;
	class DX12AssetManager;
	class HLSLMaterial;
	class HLSLShaderProgram;

	namespace RayTracing {
		class RTAccelarationStructure;
	}

	class DX12GraphicContext : public GraphicContext
	{
	public:
		DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window);
		bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory);
		virtual void Render() override;
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& mesh) override;
		virtual void AddGameEntity(ref<GameEntity>& gameEntity) override;
		virtual bool PrepareRayTracingData(const ref<ShaderProgram>& rtProgram) override;
	private:
		struct DX12GameEntityGPU {
		public:
			ComPtr<ID3D12PipelineState> pipeline;
			ref<DX12MeshController> meshController;
			ComPtr<ID3D12RootSignature> rootSignature;
			ref<HLSLMaterial> material;
			ref<Transform> transform;
		};

		void UpdateTLAS();

		UInt8 m_bufferCount;
		ComPtr<ID3D12Device10> m_device;
		ComPtr<IDXGISwapChain4> m_swapChain;
		std::vector<ComPtr<ID3D12Resource2>> m_buffers;
		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;

		ComPtr<ID3D12Resource> m_depthStencilBuffer;
		ComPtr<ID3D12DescriptorHeap> m_depthStencilvHeap;

		ref<DX12CommandExecuter> m_commandExecuter;
		ref<QuantumEngine::Platform::GraphicWindow> m_window;

		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12GraphicsCommandList7> m_commandList;
	
		ref<DX12AssetManager> m_assetManager;

		std::vector<DX12GameEntityGPU> m_entities;
		const DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D32_FLOAT;

		ComPtr<ID3D12Resource2> m_outputBuffer;
		ComPtr<ID3D12DescriptorHeap> m_outputHeap;

		ComPtr<ID3D12StateObject> m_rtStateObject; 
		ComPtr<ID3D12Resource2> m_shaderTableBuffer;

		ref<HLSLShaderProgram> m_rtProgram;
		ref<RayTracing::RTAccelarationStructure> m_TLASController;
	};
}
