#pragma once

#include "Rendering/GraphicContext.h"
#include "BasicTypes.h"
#include <vector>
#include <map>
#include "DX12LightManager.h"

#define GLOBAL_HIT_GROUP_NAME L"GlobalHit"

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
	class DX12GameEntityPipeline;

	namespace RayTracing {
		class RTAccelarationStructure;
	}

	struct DX12EntityGPUData {
	public:
		ref<GameEntity> gameEntity;
		ComPtr<ID3D12Resource2> transformResource;
	};

	struct TransformGPU {
	public:
		Matrix4 modelMatrix;
		Matrix4 rotationMatrix;
		Matrix4 modelViewMatrix;
	};

	class DX12GraphicContext : public GraphicContext
	{
	public:
		DX12GraphicContext(UInt8 bufferCount, const ref<DX12CommandExecuter>& m_commandExecuter, ref<QuantumEngine::Platform::GraphicWindow>& window);
		bool Initialize(const ComPtr<ID3D12Device10>& device, const ComPtr<IDXGIFactory7>& factory);
		virtual void Render() override;
		virtual void RegisterAssetManager(const ref<GPUAssetManager>& mesh) override;
		virtual void AddGameEntity(ref<GameEntity>& gameEntity) override;
		virtual bool PrepareRayTracingData(const ref<ShaderProgram>& rtProgram) override;
		virtual void RegisterLight(const SceneLightData& lights) override;
		void PrepareGameEntities(const std::vector<ref<GameEntity>>& gameEntities);
	private:

		struct DX12GameEntityGPU {
		public:
			ref<DX12MeshController> meshController;
			ref<HLSLMaterial> material;
			ref<Transform> transform;
			ref<HLSLMaterial> rtMaterial; 
			ComPtr<ID3D12Resource2> transformResource;
			ComPtr<ID3D12DescriptorHeap> transformHeap;
			ComPtr<ID3D12DescriptorHeap> gpuTransformHeap;
		};

		void UpdateTLAS();
		void RenderRasterization();
		void RenderRayTracing();

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
		ref<HLSLMaterial> m_rtMaterial;
		ref<RayTracing::RTAccelarationStructure> m_TLASController;
		D3D12_DISPATCH_RAYS_DESC m_raytraceDesc;
		ComPtr<ID3D12DescriptorHeap> m_rtHeap;
		TransformGPU m_transformData;

		ComPtr<ID3D12Resource2> m_cameraBuffer;
		ComPtr<ID3D12DescriptorHeap> m_cameraHeap;

		DX12LightManager m_lightManager;

		std::vector<DX12EntityGPUData> m_entityGPUData;
		std::vector<ref<DX12GameEntityPipeline>> m_rasterizationPipelines;
	};
}
