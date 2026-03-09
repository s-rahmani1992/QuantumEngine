#include "vulkan-pch.h"
#include "VulkanRayTracingPipelineModule.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/VulkanMeshController.h"
#include "Core/VulkanGraphicContext.h"
#include "Core/GameEntity.h"
#include "Core/Transform.h"
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
	auto vkDestroyAccelerationStructurePtr = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR");
	vkDestroyAccelerationStructurePtr(m_device, m_tlas, nullptr);
	vkDestroyBuffer(m_device, m_tlasBuffer, nullptr);
	vkFreeMemory(m_device, m_tlasMemory, nullptr);
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
	std::vector<VKEntityGPUData> rtEntities;

	for(auto& entityData : entities) {
		auto rtComponent = entityData.gameEntity->GetRayTracingComponent();

		if(rtComponent == nullptr)
			continue;

		rtEntities.push_back(entityData);
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
	VkDeviceAddress baseScratchAddress = scratchAddress;
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

	// Create TLAS

	std::vector<VkAccelerationStructureInstanceKHR> vkBLASInstances;
	vkBLASInstances.reserve(rtEntities.size());
	Matrix4 m;

	for(auto& entityData : rtEntities) {
		auto rtComponent = entityData.gameEntity->GetRayTracingComponent();
		auto meshController = std::dynamic_pointer_cast<VulkanMeshController>(rtComponent->GetMesh()->GetGPUHandle());
		auto& blas = m_blasMap[meshController];

		auto& blasInstance = vkBLASInstances.emplace_back(VkAccelerationStructureInstanceKHR{});

		VulkanBLASBuildInfo buildInfo = meshbuildInfos[meshController];
		UInt32 primitiveCount = buildInfo.primitiveCount;
		m = entityData.gameEntity->GetTransform()->Matrix();
		std::memcpy(&blasInstance.transform, &m, 12 * sizeof(Float));
		blasInstance.instanceCustomIndex = entityData.index;
		blasInstance.mask = 0xFF;
		blasInstance.instanceShaderBindingTableRecordOffset = 0;
		blasInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		blasInstance.accelerationStructureReference = blas->GetBLASAddress();
	}

	VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * vkBLASInstances.size();
	VkBuffer instanceBuffer;
	VkDeviceMemory instanceMemory;
	m_bufferFactory->CreateBuffer(instanceBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocFlags, &instanceBuffer, &instanceMemory);

	void* data = nullptr;
	vkMapMemory(m_device, instanceMemory, 0, instanceBufferSize, 0, &data);
	memcpy(data, vkBLASInstances.data(), instanceBufferSize);
	vkUnmapMemory(m_device, instanceMemory);

	VkBufferDeviceAddressInfo instanceBufferAddressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = instanceBuffer,
	};

	VkDeviceAddress instanceBufferAddress = vkGetBufferDeviceAddress(m_device, &instanceBufferAddressInfo);

	VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
	instancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	instancesData.arrayOfPointers = VK_FALSE;
	instancesData.data.deviceAddress = instanceBufferAddress;

	VkAccelerationStructureGeometryKHR asGeom{
		VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	asGeom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	asGeom.geometry.instances = instancesData;

	UInt32 primitiveCount = static_cast<UInt32>(rtEntities.size());

	// 3. Build sizes
	VkAccelerationStructureBuildGeometryInfoKHR TLASBuildInfo{
		VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	TLASBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	TLASBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	TLASBuildInfo.geometryCount = 1;
	TLASBuildInfo.pGeometries = &asGeom;

	VkAccelerationStructureBuildSizesInfoKHR TLASsizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	auto getRTASGetSizePtr = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR");

	getRTASGetSizePtr(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &TLASBuildInfo, &primitiveCount, &TLASsizeInfo);

	if(TLASsizeInfo.buildScratchSize > scratchBufferSize) {
		// Recreate scratch buffer
		vkDestroyBuffer(m_device, scratchBuffer, nullptr);
		vkFreeMemory(m_device, scratchMemory, nullptr);
		scratchBufferSize = TLASsizeInfo.buildScratchSize;
		m_bufferFactory->CreateBuffer(scratchBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocFlags, &scratchBuffer, &scratchMemory);
		VkBufferDeviceAddressInfo scratchAddrInfo{};
		scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		scratchAddrInfo.buffer = scratchBuffer;
		baseScratchAddress = vkGetBufferDeviceAddress(m_device, &scratchAddrInfo);
	}

	// 4. Create TLAS buffer + acceleration structure
	m_bufferFactory->CreateBuffer(TLASsizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocFlags, &m_tlasBuffer, &m_tlasMemory);

	VkAccelerationStructureCreateInfoKHR asCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	asCreateInfo.buffer = m_tlasBuffer;
	asCreateInfo.size = TLASsizeInfo.accelerationStructureSize;
	asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	auto createAccelerationStructurePtr = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR");

	createAccelerationStructurePtr(m_device, &asCreateInfo, nullptr, &m_tlas);

	VkAccelerationStructureBuildGeometryInfoKHR buildCmdInfo = TLASBuildInfo;
	buildCmdInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildCmdInfo.dstAccelerationStructure = m_tlas;
	buildCmdInfo.scratchData.deviceAddress = baseScratchAddress;

	VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
	rangeInfo.primitiveCount = primitiveCount;
	rangeInfo.primitiveOffset = 0;
	rangeInfo.firstVertex = 0;
	rangeInfo.transformOffset = 0;

	const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;
	auto buildAccelerationStructurePtr = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR");

	vkResetCommandBuffer(commandBuffer, 0);
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	buildAccelerationStructurePtr(commandBuffer, 1, &buildCmdInfo, &pRangeInfo);
	vkEndCommandBuffer(commandBuffer);

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkDestroyBuffer(m_device, instanceBuffer, nullptr);
	vkFreeMemory(m_device, instanceMemory, nullptr);
	vkDestroyBuffer(m_device, scratchBuffer, nullptr);
	vkFreeMemory(m_device, scratchMemory, nullptr);

	return true;
}
