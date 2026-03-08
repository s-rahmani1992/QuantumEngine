#include "vulkan-pch.h"
#include "VulkanRayTracingPipelineModule.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanMeshController.h"
#include "Core/VulkanGraphicContext.h"
#include "Core/GameEntity.h"
#include "Rendering/RayTracingComponent.h"
#include "Rendering/GPUMeshController.h"
#include "Core/Mesh.h"
#include "Core/VulkanBufferFactory.h"
#include "RayTracing/VulkanBLAS.h"
#include "Core/VulkanUtilities.h"

QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::VulkanRayTracingPipelineModule()
	: m_device(VulkanDeviceManager::Instance()->GetGraphicDevice()),
	m_bufferFactory(VulkanDeviceManager::Instance()->GetBufferFactory()),
	m_graphicsQueue(VulkanDeviceManager::Instance()->GetGraphicsQueue())
{
}

QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::~VulkanRayTracingPipelineModule()
{
}

bool QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::Initialize(std::vector<VKEntityGPUData>& entities)
{
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = VulkanDeviceManager::Instance()->GetGraphicsQueueFamilyIndex(),
	};

	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		return false;

	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS)
		return false;

	std::map<ref<VulkanMeshController>, VulkanBLASBuildInfo> meshbuildInfos;

	for(auto& entityData : entities) {
		auto rtComponent = entityData.gameEntity->GetRayTracingComponent();

		if(rtComponent == nullptr)
			continue;

		auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(rtComponent->GetMesh()->GetGPUHandle());
		
		auto it = meshbuildInfos.emplace(meshController, VulkanBLASBuildInfo{});
		if (it.second) {
			meshController->GetBLASBuildInfo(&((*it.first).second));
		}
	}

	UInt32 scratchOffsetAlignment = VulkanDeviceManager::Instance()->GetAccelerationStructureProperties()->minAccelerationStructureScratchOffsetAlignment;
	UInt32 scratchBufferSize = 0;

	for(auto& [meshController, buildInfo] : meshbuildInfos) {
		scratchBufferSize += ALIGN(buildInfo.sizeInfo.buildScratchSize, scratchOffsetAlignment);
	}

    VkMemoryAllocateFlagsInfo allocFlags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
    };

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchMemory;

	m_bufferFactory->CreateBuffer(scratchBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocFlags, &scratchBuffer, &scratchMemory);

    VkBufferDeviceAddressInfo scratchAddrInfo{};
    scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchAddrInfo.buffer = scratchBuffer;

    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_device, &scratchAddrInfo);

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	for(auto& [meshController, buildInfo] : meshbuildInfos) {
		auto blas = std::make_shared<VulkanBLAS>();
		buildInfo.buildInfo.scratchData.deviceAddress = scratchAddress;
		blas->CreateCommand(commandBuffer, &buildInfo);
		m_blasMap.emplace(meshController, blas);
		scratchAddress += ALIGN(buildInfo.sizeInfo.buildScratchSize, scratchOffsetAlignment);
	}

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkDestroyBuffer(m_device, scratchBuffer, nullptr);
	vkFreeMemory(m_device, scratchMemory, nullptr);

	return true;
}
