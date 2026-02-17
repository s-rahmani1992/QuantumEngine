#include "vulkan-pch.h"
#include "VulkanAssetManager.h"
#include "Core/Mesh.h"
#include "VulkanMeshController.h"

void QuantumEngine::Rendering::Vulkan::VulkanAssetManager::UploadMeshToGPU(const ref<Mesh>& mesh)
{
	ref<VulkanMeshController> meshController = std::make_shared<VulkanMeshController>(mesh, m_device);
	meshController->Initialize(m_physicalDevice);
	mesh->SetGPUHandle(meshController);
}

void QuantumEngine::Rendering::Vulkan::VulkanAssetManager::UploadTextureToGPU(const ref<Texture2D>& texture)
{
}

void QuantumEngine::Rendering::Vulkan::VulkanAssetManager::UploadMeshesToGPU(const std::vector<ref<Mesh>>& meshes)
{
}
