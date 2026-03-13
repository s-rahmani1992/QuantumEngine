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
#include "SPIRVRayTracingProgram.h"
#include "Rendering/Material.h"

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

	vkDestroyImageView(m_device, m_outputImageView, nullptr);
	vkDestroyImage(m_device, m_outputImage, nullptr);
	vkFreeMemory(m_device, m_outputImageMemory, nullptr);

	vkDestroyBuffer(m_device, m_SBT, nullptr);
	vkFreeMemory(m_device, m_SBTMemory, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::Initialize(std::vector<VKEntityGPUData>& entities, const ref<Material> rtMaterial, VkBuffer camBuffer, VkBuffer lightBuffer, UInt32 camStride, UInt32 lightStride, const VkExtent2D& extent)
{
	m_extent = extent;
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

	// create output texture
	CreateOutputImage();

	// Ray tracing pipeline
	m_globalRtProgram = std::dynamic_pointer_cast<SPIRVRayTracingProgram>(rtMaterial->GetProgram());
	CreatePipelineAndSBT();

	// Material Descriptors
	auto& reflection = m_globalRtProgram->GetReflection();
	auto& pushConstants = reflection.GetPushConstants();

	for (auto& pushConstantBlock : pushConstants.blocks)
	{
		auto valueLocation = rtMaterial->GetValueLocation(pushConstantBlock.variables[0].name);
		if (valueLocation != nullptr)
		{
			m_pushConstantValues.push_back({
				.offset = pushConstantBlock.offset,
				.location = valueLocation,
				.size = pushConstantBlock.size,
				});
		}
	}

	auto& descriptors = reflection.GetDescriptors();
	auto textureFields = rtMaterial->GetTextureFields();
	UInt32 fieldIndex = 0;

	for (auto& descriptor : descriptors) {
		if (descriptor.name[0] == '_')
			continue;

		m_descriptorData.push_back(DescriptorData{
			.setIndex = descriptor.data.set,
			.binding = descriptor.data.binding,
			.descriptorType = descriptor.descriptorType,
			});

		auto matIt = textureFields->find(descriptor.name);

		if (matIt != textureFields->end())
			(*matIt).second.fieldIndex = fieldIndex;

		fieldIndex++;
	}


	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(20);
	UInt32 setcount = reflection.GetDescriptorLayoutCount();

	for (auto& descriptor : descriptors) {
		auto it = std::find_if(poolSizes.begin(), poolSizes.end(), [descriptor](const VkDescriptorPoolSize& poolSize) {
			return descriptor.descriptorType == poolSize.type;
			});

		if (it != poolSizes.end())
			(*it).descriptorCount++;
		else
			poolSizes.push_back(VkDescriptorPoolSize{
			.type = descriptor.descriptorType,
			.descriptorCount = 1,
				});
	}

	poolSizes.push_back(VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = 3,
		});

	poolSizes.push_back(VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 3,
		});

	VkDescriptorPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = setcount,
		.poolSizeCount = (UInt32)poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(m_device, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		return false;

	auto& layouts = m_globalRtProgram->GetDiscriptorLayouts();
	m_descriptorSets.resize(layouts.size());
	m_descriptorData.resize(layouts.size());

	VkDescriptorSetAllocateInfo descSetAlloc{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = m_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = nullptr,
	};

	for (UInt32 i = 0; i < layouts.size(); i++) {
		descSetAlloc.pSetLayouts = layouts.data() + i;

		if (vkAllocateDescriptorSets(m_device, &descSetAlloc, m_descriptorSets.data() + i) != VK_SUCCESS)
			return false;
	}

	WriteBuffers(HLSL_CAMERA_DATA_NAME, camBuffer, camStride);
	WriteBuffers(HLSL_LIGHT_DATA_NAME, lightBuffer, lightStride);

	auto descriptorData = m_globalRtProgram->GetReflection().GetDescriptorData(HLSL_RT_OUTPUT_TEXTURE_NAME);

	if (descriptorData != nullptr) {
		VkDescriptorImageInfo imgDesc{};
		imgDesc.imageView = m_outputImageView;
		imgDesc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_descriptorSets[descriptorData->data.set];
		write.dstBinding = descriptorData->data.binding;
		write.descriptorType = descriptorData->descriptorType;
		write.descriptorCount = 1;
		write.pImageInfo = &imgDesc;

		vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	}

	descriptorData = m_globalRtProgram->GetReflection().GetDescriptorData(HLSL_RT_TLAS_SCENE_NAME);
	
	if (descriptorData != nullptr) {
		VkWriteDescriptorSetAccelerationStructureKHR asInfo{};
		asInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		asInfo.accelerationStructureCount = 1;
		asInfo.pAccelerationStructures = &m_tlas; // VkAccelerationStructureKHR

		VkWriteDescriptorSet writeAS{};
		writeAS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeAS.dstSet = m_descriptorSets[descriptorData->data.set];
		writeAS.dstBinding = descriptorData->data.binding;
		writeAS.descriptorCount = 1;
		writeAS.descriptorType = descriptorData->descriptorType;
		writeAS.pNext = &asInfo;

		vkUpdateDescriptorSets(m_device, 1, &writeAS, 0, nullptr);
	}

	vkDestroyBuffer(m_device, instanceBuffer, nullptr);
	vkFreeMemory(m_device, instanceMemory, nullptr);
	vkDestroyBuffer(m_device, scratchBuffer, nullptr);
	vkFreeMemory(m_device, scratchMemory, nullptr);
	vkDestroyCommandPool(m_device, commandPool, nullptr);

	return true;
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::RenderCommand(VkCommandBuffer commandBuffer)
{
	// Ensure the ray-tracing output image is in a layout the shaders can write to.
	// The image was created with undefined layout; transition it to GENERAL before tracing.
	VkImageMemoryBarrier imageBarrier{};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.srcAccessMask = 0;
	imageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = m_outputImage;
	imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;

	// Transition to GENERAL before ray tracing shaders write to it.
	// Use TOP_OF_PIPE -> RAY_TRACING_SHADER stage to guarantee availability.
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
		0, 0, nullptr, 0, nullptr, 1, &imageBarrier);


	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);

	int shaderFlag = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	for (auto& pushConstant : m_pushConstantValues)
	{
		vkCmdPushConstants(commandBuffer, m_globalRtProgram->GetPipelineLayout(), shaderFlag, pushConstant.offset, pushConstant.size, pushConstant.location);
	}

	UInt32 offsets[2] = { 0, 0 };

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_globalRtProgram->GetPipelineLayout(), 0, (UInt32)m_descriptorSets.size(), m_descriptorSets.data(), 1, offsets);

	auto vkCmdTraceRaysPtr = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR");

	vkCmdTraceRaysPtr(commandBuffer, &m_rayGenRegion, &m_missRegion, &m_hitRegion, &m_callableRegion, m_extent.width, m_extent.height, 1);
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::CreateOutputImage()
{
	VkImageCreateInfo imgInfo{};
	imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgInfo.imageType = VK_IMAGE_TYPE_2D;
	imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imgInfo.extent = { m_extent.width, m_extent.height, 1 };
	imgInfo.mipLevels = 1;
	imgInfo.arrayLayers = 1;
	imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imgInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	vkCreateImage(m_device, &imgInfo, nullptr, &m_outputImage);

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(m_device, m_outputImage, &memReq);

	VkMemoryAllocateInfo imAllocInfo{};
	imAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imAllocInfo.allocationSize = memReq.size;
	imAllocInfo.memoryTypeIndex = GetMemoryTypeIndex(&memReq, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_bufferFactory->GetMemoryProperties());

	vkAllocateMemory(m_device, &imAllocInfo, nullptr, &m_outputImageMemory);
	vkBindImageMemory(m_device, m_outputImage, m_outputImageMemory, 0);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_outputImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imgInfo.format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(m_device, &viewInfo, nullptr, &m_outputImageView);
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::CreatePipelineAndSBT()
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;

	auto rtGlobalStages = m_globalRtProgram->GetShaderStages();
	stages.insert(stages.end(), rtGlobalStages.begin(), rtGlobalStages.end());

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

	groups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		.generalShader = 0, // Ray Gen shader index
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
		});

	groups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		.generalShader = 1, // Miss shader index
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
		});

	groups.push_back(VkRayTracingShaderGroupCreateInfoKHR{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
		.generalShader = VK_SHADER_UNUSED_KHR,
		.closestHitShader = 2, // Closest Hit shader index
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
		});

	VkRayTracingPipelineCreateInfoKHR rtPipelineInfo{
	.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
	.stageCount = (UInt32)stages.size(),
	.pStages = stages.data(),
	.groupCount = (uint32_t)groups.size(),
	.pGroups = groups.data(),
	.maxPipelineRayRecursionDepth = 7,
	.layout = m_globalRtProgram->GetPipelineLayout(),
	};

	auto createRayTracingPipelinesPtr = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR");

	createRayTracingPipelinesPtr(m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtPipelineInfo, nullptr, &m_rtPipeline);

	// Create SBT
	auto rtProperties = VulkanDeviceManager::Instance()->GetRayTracingPipelineProperties();

	UInt32 groupCount = (UInt32)groups.size(); // raygen + miss + hit groups
	UInt32 dataSize = groupCount * rtProperties->shaderGroupHandleSize;

	std::vector<UInt8> shaderHandleStorage(dataSize);

	auto getShaderGroupHandlesPtr = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR");

	getShaderGroupHandlesPtr(m_device, m_rtPipeline, 0, groupCount, dataSize, shaderHandleStorage.data());
	
	UInt32 groupHandleSize = rtProperties->shaderGroupHandleSize;
	UInt32 groupHandleAlignment = rtProperties->shaderGroupHandleAlignment;
	UInt32 baseAlignment = rtProperties->shaderGroupBaseAlignment;

	UInt32 raygenSize = ALIGN(groupHandleSize, groupHandleAlignment);
	UInt32 missSize = ALIGN(groupHandleSize, groupHandleAlignment);
	UInt32 hitSize = ALIGN(groupHandleSize, groupHandleAlignment);
	UInt32 callableSize = 0;  // No callable shaders for now
	
	// Ensure each region starts at a baseAlignment boundary
	UInt32 raygenOffset = 0;
	UInt32 missOffset = ALIGN(raygenSize, baseAlignment);
	UInt32 hitOffset = ALIGN(missOffset + missSize, baseAlignment);
	UInt32 callableOffset = ALIGN(hitOffset + hitSize, baseAlignment);

	auto SBTBufferSize = callableOffset + callableSize;
	
	VkMemoryAllocateFlagsInfo allocFlags{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
	};

	m_bufferFactory->CreateBuffer(SBTBufferSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocFlags, &m_SBT, &m_SBTMemory);

	// map and copy handles at aligned offset
	VkBufferDeviceAddressInfo addrInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = m_SBT
	};

	VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(m_device, &addrInfo);

	void* mapped;
	vkMapMemory(m_device, m_SBTMemory, 0, VK_WHOLE_SIZE, 0, &mapped);

	UInt8* dst = (UInt8*)(mapped);

	memcpy(dst + raygenOffset, shaderHandleStorage.data(), groupHandleSize);
	memcpy(dst + missOffset, shaderHandleStorage.data() + groupHandleSize, groupHandleSize);
	memcpy(dst + hitOffset, shaderHandleStorage.data() + 2 * groupHandleSize, groupHandleSize);
	vkUnmapMemory(m_device, m_SBTMemory);


	m_rayGenRegion.deviceAddress = sbtAddress + raygenOffset;
	m_rayGenRegion.stride = raygenSize;
	m_rayGenRegion.size = raygenSize;

	m_missRegion.deviceAddress = sbtAddress + missOffset;
	m_missRegion.stride = missSize;
	m_missRegion.size = missSize;

	m_hitRegion.deviceAddress = sbtAddress + hitOffset;
	m_hitRegion.stride = hitSize;
	m_hitRegion.size = hitSize;
	
	int h = 0;
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingPipelineModule::WriteBuffers(const std::string name, const VkBuffer buffer, UInt32 stride)
{
	auto descriptorData = m_globalRtProgram->GetReflection().GetDescriptorData(name);

	if (descriptorData == nullptr)
		return;

	VkDescriptorBufferInfo descBufferInfo{
		.buffer = buffer,
		.offset = 0,
		.range = stride,
	};

	VkWriteDescriptorSet writeDescriptor{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptorSets[descriptorData->data.set],
		.dstBinding = descriptorData->data.binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = descriptorData->descriptorType,
		.pImageInfo = nullptr,
		.pBufferInfo = &descBufferInfo,
		.pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(m_device, 1, &writeDescriptor, 0, nullptr);
}
