#include "vulkan-pch.h"
#include "VulkanGraphicContext.h"
#include "Platform/GraphicWindow.h"

QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::VulkanGraphicContext(const VkInstance vkInstance, VkPhysicalDevice physicalDevice, VkDevice logicDevice, UInt32 graphicsQueueFamilyIndex, UInt32 surfaceQueueFamilyIndex, const ref<Platform::GraphicWindow>& window)
	:m_instance(vkInstance), m_physicalDevice(physicalDevice), m_logicDevice(logicDevice), 
	m_graphicsQueueFamilyIndex(graphicsQueueFamilyIndex), m_window(window)
{
	vkGetDeviceQueue(m_logicDevice, graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicDevice, surfaceQueueFamilyIndex, 0, &m_presentQueue);
}

QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::~VulkanGraphicContext()
{
	vkDestroySemaphore(m_logicDevice, m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_logicDevice, m_renderFinishedSemaphore, nullptr);
	vkDestroyFence(m_logicDevice, m_fence, nullptr);
	vkDestroyCommandPool(m_logicDevice, m_commandPool, nullptr);

	for (auto framebuffer : m_swapChainFramebuffers) {
		vkDestroyFramebuffer(m_logicDevice, framebuffer, nullptr);
	}

	vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);

	for(auto& imageView : m_swapChainImageViews) {
		vkDestroyImageView(m_logicDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_logicDevice, m_swapChain, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::Initialize()
{
	// Create Surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = m_window->GetHandle(),
	};

	VkResult result = vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface);

	if (result != VK_SUCCESS)
		return false;

	// Create Swap Chain
	uint32_t formatCount;
	std::vector<VkSurfaceFormatKHR> formats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
	formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

	for (const auto& availableFormat : formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			m_swapChainFormat = availableFormat;
			break;
		}
	}

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &m_swapChainCapability);

	VkSwapchainCreateInfoKHR swapChainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0, // TODO Check if any flags are needed
		.surface = m_surface,
		.minImageCount = m_swapChainCapability.minImageCount,
		.imageFormat = m_swapChainFormat.format,
		.imageColorSpace = m_swapChainFormat.colorSpace,
		.imageExtent = m_swapChainCapability.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.preTransform = m_swapChainCapability.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	result = vkCreateSwapchainKHR(m_logicDevice, &swapChainCreateInfo, nullptr, &m_swapChain);

	if (result != VK_SUCCESS)
		return false;


	// Create Image Views for Swap chain images
	UInt32 imageCount;
	vkGetSwapchainImagesKHR(m_logicDevice, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_logicDevice, m_swapChain, &imageCount, m_swapChainImages.data());
	m_swapChainImageViews.resize(imageCount);

	for (UInt32 i = 0; i < m_swapChainImages.size(); i++) {
		VkImageViewCreateInfo imageViewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0, // TODO Check if any flags are needed
			.image = m_swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_swapChainFormat.format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		vkCreateImageView(m_logicDevice, &imageViewCreateInfo, nullptr, &m_swapChainImageViews[i]);
	}

	// Create Render Pass
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

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
	};

	VkRenderPassCreateInfo rpInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	result = vkCreateRenderPass(m_logicDevice, &rpInfo, nullptr, &m_renderPass);
	if (result != VK_SUCCESS)
		return false;

	// Create Framebuffers for Swap chain images
	m_swapChainFramebuffers.resize(imageCount);
	VkFramebuffer* framebuffer = m_swapChainFramebuffers.data();

	for (auto view : m_swapChainImageViews) {
		VkFramebufferCreateInfo fbInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = m_renderPass,
			.attachmentCount = 1,
			.pAttachments = &view,
			.width = m_swapChainCapability.currentExtent.width,
			.height = m_swapChainCapability.currentExtent.height,
			.layers = 1,
		};
		vkCreateFramebuffer(m_logicDevice, &fbInfo, nullptr, framebuffer);
		framebuffer++;
	}

	// Create Command Pool and Command Buffer
	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_graphicsQueueFamilyIndex,
	};

	result = vkCreateCommandPool(m_logicDevice, &poolInfo, nullptr, &m_commandPool);
	if (result != VK_SUCCESS)
		return false;

	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = m_commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	result = vkAllocateCommandBuffers(m_logicDevice, &allocInfo, &m_commandBuffer);
	if (result != VK_SUCCESS)
		return false;

	// Create Fences and Semaphores
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0, // TODO Check if any flags are needed
	};

	result = vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore);
	if (result != VK_SUCCESS)
		return false;

	result = vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);
	if (result != VK_SUCCESS)
		return false;

	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};

	result = vkCreateFence(m_logicDevice, &fenceInfo, nullptr, &m_fence);
	if (result != VK_SUCCESS)
		return false;

	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::RegisterAssetManager(const ref<GPUAssetManager>& assetManager)
{
}

void QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::RegisterShaderRegistery(const ref<ShaderRegistery>& shaderRegistery)
{
}

bool QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::PrepareScene(const ref<Scene>& scene)
{
	return false;
}

void QuantumEngine::Rendering::Vulkan::VulkanGraphicContext::Render()
{
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
	VkClearValue clearColor = { .color = { {0.5f, 0.6f, 0.1f, 1.0f} } };
	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_renderPass,
		.framebuffer = m_swapChainFramebuffers[imageIndex],
		.renderArea = {
			.offset = { 0, 0 },
			.extent = m_swapChainCapability.currentExtent,
		},
		.clearValueCount = 1,
		.pClearValues = &clearColor,
	};

	vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Render Objects here

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
