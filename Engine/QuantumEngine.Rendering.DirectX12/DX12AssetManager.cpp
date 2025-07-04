#include "pch.h"
#include "DX12AssetManager.h"
#include "DX12MeshController.h"
#include "DX12Texture2DController.h"
#include "DX12CommandExecuter.h"
#include "Core/Texture2D.h"

void QuantumEngine::Rendering::DX12::DX12AssetManager::UploadMeshToGPU(const ref<Mesh>& mesh)
{
	if (m_meshes.find(mesh) != m_meshes.end()) { // mesh has already been uploaded
		return;
	}

	auto meshController = std::make_shared<DX12MeshController>(mesh);

	if (meshController->Initialize(m_device) == false)
		return;

	// Reset Commands
	m_uploadCommandAllocator->Reset();
	m_uploadCommandList->Reset(m_uploadCommandAllocator.Get(), nullptr);

	// Execute Upload Commands
	meshController->UploadToGPU(m_uploadCommandList);
	m_meshUploadCommandExecuter->ExecuteAndWait(m_uploadCommandList.Get());
	m_meshes.insert({ mesh, meshController });
}

void QuantumEngine::Rendering::DX12::DX12AssetManager::UploadTextureToGPU(const ref<Texture2D>& texture)
{
	if (m_textures.find(texture) != m_textures.end()) { // mesh has already been uploaded
		return;
	}

	auto texture2DController = std::make_shared<DX12Texture2DController>(texture);

	if (texture2DController->Initialize(m_device) == false)
		return;

	// Reset Commands
	m_uploadCommandAllocator->Reset();
	m_uploadCommandList->Reset(m_uploadCommandAllocator.Get(), nullptr);

	// Execute Upload Commands
	texture2DController->UploadToGPU(m_uploadCommandList);
	m_meshUploadCommandExecuter->ExecuteAndWait(m_uploadCommandList.Get());
	texture->SetGPUHandle(texture2DController);
	m_textures.insert({ texture, texture2DController });
}

bool QuantumEngine::Rendering::DX12::DX12AssetManager::Initialize(ComPtr<ID3D12Device10>& device)
{
	m_device = device;

	//Command Queue
	ComPtr<ID3D12CommandQueue> cmdqueue;
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{
	.Type = D3D12_COMMAND_LIST_TYPE_COPY,
	.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
	.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	.NodeMask = 0,
	};

	if (FAILED(device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdqueue))))
		return false;

	//fence
	ComPtr<ID3D12Fence1> fence;
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
		return false;

	m_meshUploadCommandExecuter = std::make_shared<DX12CommandExecuter>(cmdqueue, fence);
	
	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_uploadCommandAllocator))))
		return false;

	if (FAILED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_uploadCommandList))))
		return false;

	return true;
}
