#include "vulkan-pch.h"
#include "VulkanRayTracingContext.h"
#include <set>
#include "Core/GameEntity.h"
#include "Rendering/RayTracingComponent.h"
#include "Core/Scene.h"
#include "Core/VulkanAssetManager.h"
#include "RayTracing/VulkanRayTracingPipelineModule.h"
#include "Core/VulkanBufferFactory.h"
#include "Core/VulkanDeviceManager.h"
#include "Core/Transform.h"

QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingContext::VulkanRayTracingContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window)
	: VulkanGraphicContext(vkInstance, surfaceQueueFamilyIndex, window),
	m_rayTracingModule(std::make_shared<RayTracing::VulkanRayTracingPipelineModule>())
{
}

bool QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingContext::Initialize()
{
	if (InitializeSwapChain(VK_IMAGE_USAGE_TRANSFER_DST_BIT) == false)
		return false;

	if (InitializeCommandObjects() == false)
		return false;

	if (InitializeFencesAndSemaphores() == false)
		return false;

	return true;
}

bool QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingContext::PrepareScene(const ref<Scene>& scene)
{
	UploadMeshes(scene->entities);

	if (InitializeCameraBuffer(scene->mainCamera) == false)
		return false;

	if (InitializeLightBuffer(scene->lightData) == false)
		return false;

    auto bufferFactory = VulkanDeviceManager::Instance()->GetBufferFactory();
    bufferFactory->CreateBuffer(sizeof(TransformGPU) * scene->entities.size()
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_transformBuffer, &m_transformBufferMemory);

    UInt32 index = 0;
    m_entityGPUList.reserve(scene->entities.size());

    for (auto& entity : scene->entities) {
        m_entityGPUList.push_back({ entity, index });
        index++;
    }

	if(m_rayTracingModule->Initialize(scene->entities, scene->rtGlobalMaterial, m_cameraBuffer, m_lightBuffer, m_transformBuffer, m_swapChainCapability.currentExtent) == false)
		return false;

	return true;
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingContext::Render()
{
    UpdateCameraBuffer();
    UpdateTransforms();

    vkResetFences(m_logicDevice, 1, &m_fence);

    UInt32 imageIndex;
    vkAcquireNextImageKHR(m_logicDevice, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(m_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    
    m_rayTracingModule->UpdateTLAS(m_commandBuffer);
	m_rayTracingModule->RenderCommand(m_commandBuffer);

    VkImageMemoryBarrier rtToSrc = {};
    rtToSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    rtToSrc.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    rtToSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    rtToSrc.oldLayout = VK_IMAGE_LAYOUT_GENERAL; // typical RT output layout
    rtToSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    rtToSrc.image = m_rayTracingModule->GetOutputImage();
    rtToSrc.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        m_commandBuffer,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &rtToSrc
    );

    // 2. Transition swapchain image ? TRANSFER_DST
    VkImageMemoryBarrier swapToDst = {};
    swapToDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapToDst.srcAccessMask = 0;
    swapToDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapToDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // safe for acquired images
    swapToDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapToDst.image = m_swapChainImages[imageIndex];
    swapToDst.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        m_commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &swapToDst
    );

    // 3. Blit or copy ray output ? swapchain
	int width = m_swapChainCapability.currentExtent.width;
	int height = m_swapChainCapability.currentExtent.height;
    VkImageBlit blit = {};
    blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.srcOffsets[1] = { width, height, 1 };
    blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.dstOffsets[1] = { width, height, 1 };

    vkCmdBlitImage(
        m_commandBuffer,
        m_rayTracingModule->GetOutputImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        m_swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit,
        VK_FILTER_LINEAR
    );

    // 4. Transition swapchain image ? PRESENT
    VkImageMemoryBarrier swapToPresent = {};
    swapToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapToPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapToPresent.dstAccessMask = 0;
    swapToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapToPresent.image = m_swapChainImages[imageIndex];
    swapToPresent.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        m_commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &swapToPresent
    );

    vkEndCommandBuffer(m_commandBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_imageAvailableSemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_renderFinishedSemaphore,
    };

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_fence);

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &m_swapChain,
        .pImageIndices = &imageIndex,
    };

    vkQueuePresentKHR(m_presentQueue, &presentInfo);
    vkQueueWaitIdle(m_presentQueue);

    vkWaitForFences(m_logicDevice, 1, &m_fence, VK_TRUE, 20000);
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingContext::UploadMeshes(const std::vector<ref<GameEntity>>& entities)
{
	std::set<ref<Mesh>> uniqueMeshes;

	for (auto& entity : entities) {
		auto rtcomponent = entity->GetRayTracingComponent();

		if (rtcomponent != nullptr)
			uniqueMeshes.insert(rtcomponent->GetMesh());
	}

	m_assetManager->UploadMeshesToGPU(std::vector<ref<Mesh>>(uniqueMeshes.begin(), uniqueMeshes.end()));
}

void QuantumEngine::Rendering::Vulkan::RayTracing::VulkanRayTracingContext::UpdateTransforms()
{
    void* data;
    vkMapMemory(m_logicDevice, m_transformBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);

    for (auto& entityGPU : m_entityGPUList) {
        m_transformData.modelMatrix = entityGPU.gameEntity->GetTransform()->Matrix();
        m_transformData.modelViewMatrix = m_cameraGPU.viewMatrix * m_transformData.modelMatrix;
        m_transformData.rotationMatrix = entityGPU.gameEntity->GetTransform()->RotateMatrix();

        std::memcpy((Byte*)data + entityGPU.index * sizeof(TransformGPU), &m_transformData, sizeof(TransformGPU));
    }

    vkUnmapMemory(m_logicDevice, m_transformBufferMemory);
}
