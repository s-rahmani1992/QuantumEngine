#include "vulkan-pch.h"
#include "VulkanHybridContext.h"
#include "Core/Scene.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "Core/Transform.h"
#include "Rendering/Renderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/SplineRenderer.h"
#include "Rendering/RayTracingComponent.h"
#include "VulkanAssetManager.h"
#include "VulkanUtilities.h"
#include "VulkanShaderRegistery.h"
#include "VulkanBufferFactory.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "Rasterization/VulkanRasterizationMaterial.h"
#include "Rasterization/VulkanRasterizationPipelineModule.h"
#include "VulkanSplinePipelineModule.h"
#include "Compute/SPIRVComputeProgram.h"

QuantumEngine::Rendering::Vulkan::VulkanHybridContext::VulkanHybridContext(const VkInstance vkInstance, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window)
	:VulkanGraphicContext(vkInstance, surfaceQueueFamilyIndex, window)
{
}

QuantumEngine::Rendering::Vulkan::VulkanHybridContext::~VulkanHybridContext()
{
	vkDestroyBuffer(m_logicDevice, m_transformBuffer, nullptr);
	vkFreeMemory(m_logicDevice, m_transformBufferMemory, nullptr);

	vkDestroyImage(m_logicDevice, m_depthImage, nullptr);
	vkFreeMemory(m_logicDevice, m_depthMemory, nullptr);

	for (auto framebuffer : m_swapChainFramebuffers) {
		vkDestroyFramebuffer(m_logicDevice, framebuffer, nullptr);
	}

	vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::VulkanHybridContext::Initialize()
{
	if (InitializeSwapChain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == false)
		return false;

	if (InitializeDepthBuffer() == false)
		return false;

	if(InitializeRenderPass() == false)
		return false;

	if(InitializeCommandObjects() == false)
		return false;

	if(InitializeFencesAndSemaphores() == false)
		return false;

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanHybridContext::PrepareScene(const ref<Scene>& scene)
{
	UploadMeshesToGPU(scene->entities);

	if(InitializeCameraBuffer(scene->mainCamera) == false)
		return false;

	if(InitializeLightBuffer(scene->lightData) == false)
		return false;

	// Create Uniform Buffers for Transforms
	m_bufferFactory->CreateBuffer(sizeof(TransformGPU), scene->entities.size()
		, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_transformBuffer, &m_transformBufferMemory, &m_transformStride);


	std::map<ref<Material>, ref<Rasterization::VulkanRasterizationMaterial>> usedMaterials;
	UInt32 index = 0;
	m_entityGPUList.reserve(scene->entities.size());


	for (auto& entity : scene->entities) {
		m_entityGPUList.push_back({ entity, index });

		auto material = entity->GetRenderer()->GetMaterial();
		auto matIT = usedMaterials.find(material);

		if (matIT == usedMaterials.end()) {
			usedMaterials.emplace(material, std::make_shared<Rasterization::VulkanRasterizationMaterial>(material, m_logicDevice)).first;
		}

		index++;
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(20);
	UInt32 setcount = 0;

	for (auto& matPair : usedMaterials) {
		auto program = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>(matPair.first->GetProgram());
		auto& reflection = program->GetReflection();
		setcount += reflection.GetDescriptorLayoutCount();
		auto& descriptors = reflection.GetDescriptors();

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
	}

	setcount += m_entityGPUList.size();

	poolSizes.push_back(VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = (UInt32)usedMaterials.size(),
		});

	poolSizes.push_back(VkDescriptorPoolSize{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = (UInt32)m_entityGPUList.size(),
		});

	VkDescriptorPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = setcount,
		.poolSizeCount = (UInt32)poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	if (vkCreateDescriptorPool(m_logicDevice, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		return false;

	for (auto& entityGPU : m_entityGPUList) {
		auto& entity = entityGPU.gameEntity;
		auto Renderer = entity->GetRenderer();

		auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(Renderer);

		if (meshRenderer != nullptr) {
			auto& gpuMateiral = usedMaterials[meshRenderer->GetMaterial()];
			ref<Rasterization::VulkanRasterizationPipelineModule> rasterizationModule = std::make_shared<Rasterization::VulkanRasterizationPipelineModule>(m_logicDevice);
			if (rasterizationModule->Initialize(entity, gpuMateiral, m_renderPass)) {
				m_rasterizationModules.push_back(rasterizationModule);
				rasterizationModule->SetDescriptorOffset(HLSL_OBJECT_TRANSFORM_DATA_NAME, entityGPU.index * m_transformStride);
				rasterizationModule->SetDescriptorOffset(HLSL_CAMERA_DATA_NAME, 0);
				rasterizationModule->SetDescriptorOffset(HLSL_LIGHT_DATA_NAME, 0);
			}
		}

		auto splineRenderer = std::dynamic_pointer_cast<SplineRenderer>(Renderer);

		if (splineRenderer != nullptr) {
			auto& gpuMateiral = usedMaterials[splineRenderer->GetMaterial()];
			ref<VulkanSplinePipelineModule> splineModule = std::make_shared<VulkanSplinePipelineModule>(m_logicDevice);
			SplineEntityData splineEntityData{
				.splineRenderer = splineRenderer,
				.material = gpuMateiral,
				.computeProgram = std::dynamic_pointer_cast<Compute::SPIRVComputeProgram>(m_shaderRegistery->GetShaderPrograms("Bezier_Curve_Compute_Program")),
			};
			if (splineModule->Initialize(splineEntityData, m_renderPass, m_descriptorPool)) {
				m_splineModues.push_back(splineModule);
				splineModule->WriteOffset(HLSL_OBJECT_TRANSFORM_DATA_NAME, entityGPU.index * m_transformStride);
				splineModule->WriteOffset(HLSL_CAMERA_DATA_NAME, 0);
				splineModule->WriteOffset(HLSL_LIGHT_DATA_NAME, 0);
			}
		}
	}

	for (auto& matPair : usedMaterials) {
		if (matPair.second->Initialize(m_descriptorPool) == false)
			continue;
		matPair.second->WriteBuffer(HLSL_OBJECT_TRANSFORM_DATA_NAME, m_transformBuffer, m_transformStride);
		matPair.second->WriteBuffer(HLSL_CAMERA_DATA_NAME, m_cameraBuffer, m_cameraStride);
		matPair.second->WriteBuffer(HLSL_LIGHT_DATA_NAME, m_lightBuffer, m_lightStride);
	}


	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanHybridContext::Render()
{
	UpdateCameraBuffer();
	UpdateEntityTransforms();

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

	for (auto& m_splineModues : m_splineModues) {
		m_splineModues->ComputeCommand(m_commandBuffer);
	}

	VkClearValue clearColor = { .color = { {0.5f, 0.6f, 0.1f, 1.0f} } };

	VkClearValue clearValues[2];
	clearValues[0].color = { {0.5f, 0.6f, 0.1f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_renderPass,
		.framebuffer = m_swapChainFramebuffers[imageIndex],
		.renderArea = {
			.offset = { 0, 0 },
			.extent = m_swapChainCapability.currentExtent,
		},
		.clearValueCount = 2,
		.pClearValues = clearValues,
	};

	VkRenderPassBeginInfo middleRenderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_renderPass,
		.framebuffer = m_swapChainFramebuffers[imageIndex],
		.renderArea = {
			.offset = { 0, 0 },
			.extent = m_swapChainCapability.currentExtent,
		},
		.clearValueCount = 0,
		.pClearValues = nullptr,
	};

	vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChainCapability.currentExtent.width;
	viewport.height = (float)m_swapChainCapability.currentExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainCapability.currentExtent;

	vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);


	// Render Objects here
	for (auto& module : m_rasterizationModules) {
		module->RenderCommand(m_commandBuffer);
	}

	for (auto& module : m_splineModues) {
		module->RenderCommand(m_commandBuffer);
	}

	vkCmdEndRenderPass(m_commandBuffer);
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

void QuantumEngine::Rendering::Vulkan::VulkanHybridContext::UploadMeshesToGPU(const std::vector<ref<GameEntity>>& entities)
{
	std::set<ref<Mesh>> uniqueMeshes;

	for (auto& entity : entities) {
		auto mesh = entity->GetRenderer()->GetMesh();

		if (mesh == nullptr)
			continue;

		uniqueMeshes.insert(mesh);
		auto rtcomponent = entity->GetRayTracingComponent();

		if (rtcomponent != nullptr)
			uniqueMeshes.insert(rtcomponent->GetMesh());
	}

	m_assetManager->UploadMeshesToGPU(std::vector<ref<Mesh>>(uniqueMeshes.begin(), uniqueMeshes.end()));
}

bool QuantumEngine::Rendering::Vulkan::VulkanHybridContext::InitializeDepthBuffer()
{
	// Create depth image
	VkImageCreateInfo imgInfo{
	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	.imageType = VK_IMAGE_TYPE_2D,
	.format = m_depthFormat,
	.extent = { m_swapChainCapability.currentExtent.width, m_swapChainCapability.currentExtent.height, 1 },
	.mipLevels = 1,
	.arrayLayers = 1,
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.tiling = VK_IMAGE_TILING_OPTIMAL,
	.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	m_bufferFactory->CreateImage(&imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_depthImage, &m_depthMemory);

	// Create depth image view
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(m_logicDevice, &viewInfo, nullptr, &m_depthImageView);

	return true;
}

bool QuantumEngine::Rendering::Vulkan::VulkanHybridContext::InitializeRenderPass()
{
	VkAttachmentDescription colorAttachment{
		.format = m_swapChainFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentDescription depthAttachment{
		.format = m_depthFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription colorSubpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	VkRenderPassCreateInfo rpInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &colorSubpass,
	};

	auto result = vkCreateRenderPass(m_logicDevice, &rpInfo, nullptr, &m_renderPass);
	if (result != VK_SUCCESS)
		return false;

	// Create Framebuffers for Swap chain images
	m_swapChainFramebuffers.resize(2);
	VkFramebuffer* framebuffer = m_swapChainFramebuffers.data();

	for (auto view : m_swapChainImageViews) {
		VkImageView imageViews[2] = { view, m_depthImageView };

		VkFramebufferCreateInfo fbInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_renderPass,
			.attachmentCount = 2,
			.pAttachments = imageViews,
			.width = m_swapChainCapability.currentExtent.width,
			.height = m_swapChainCapability.currentExtent.height,
			.layers = 1,
		};
		vkCreateFramebuffer(m_logicDevice, &fbInfo, nullptr, framebuffer);
		framebuffer++;
	}
	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanHybridContext::UpdateEntityTransforms()
{
	void* data;
	vkMapMemory(m_logicDevice, m_transformBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);

	for (auto& entityGPU : m_entityGPUList) {
		m_transformData.modelMatrix = entityGPU.gameEntity->GetTransform()->Matrix();
		m_transformData.modelViewMatrix = m_cameraGPU.viewMatrix * m_transformData.modelMatrix;
		m_transformData.rotationMatrix = entityGPU.gameEntity->GetTransform()->RotateMatrix();

		std::memcpy((Byte*)data + entityGPU.index * m_transformStride, &m_transformData, sizeof(TransformGPU));
	}

	vkUnmapMemory(m_logicDevice, m_transformBufferMemory);
}
