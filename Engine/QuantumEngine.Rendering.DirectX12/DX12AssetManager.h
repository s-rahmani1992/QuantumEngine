#pragma once

#include "Rendering/GPUAssetManager.h"
#include <map>

namespace QuantumEngine {
	class Mesh;
	class Texture2D;
}

using namespace Microsoft::WRL;

namespace QuantumEngine::Rendering::DX12 {
	class DX12MeshController;
	class DX12CommandExecuter;
	class DX12Texture2DController;

	class DX12AssetManager : public GPUAssetManager
	{
	public:
		virtual void UploadMeshToGPU(const ref<Mesh>& mesh) override;
		virtual void UploadTextureToGPU(const ref<Texture2D>& texture) override;
		bool Initialize(ComPtr<ID3D12Device10>& device);
		ref<DX12MeshController> GetMeshController(const ref<Mesh>& mesh) { return m_meshes.at(mesh); }
	private:
		ComPtr<ID3D12Device10> m_device;
		ComPtr<ID3D12CommandAllocator> m_uploadCommandAllocator;
		ComPtr<ID3D12GraphicsCommandList7> m_uploadCommandList;
		ref<DX12CommandExecuter> m_meshUploadCommandExecuter;
		std::map<ref<Mesh>, ref<DX12MeshController>> m_meshes;
		std::map<ref<Texture2D>, ref<DX12Texture2DController>> m_textures;
	};
}
