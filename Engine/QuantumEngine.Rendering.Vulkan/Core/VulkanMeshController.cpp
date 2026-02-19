#include "vulkan-pch.h"
#include "VulkanMeshController.h"
#include "Core/Mesh.h"
#include "VulkanUtilities.h"

QuantumEngine::Rendering::Vulkan::VulkanMeshController::VulkanMeshController(const ref<Mesh>& mesh, const VkDevice device)
	: m_mesh(mesh), m_device(device)
{
}

QuantumEngine::Rendering::Vulkan::VulkanMeshController::~VulkanMeshController()
{
	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
	vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
}

bool QuantumEngine::Rendering::Vulkan::VulkanMeshController::Initialize(const VkPhysicalDeviceMemoryProperties* memoryProperties)
{
	// Create Vertex Buffer
	VkBufferCreateInfo vBufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = sizeof(Vertex) * m_mesh->GetVertexCount(),
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	if (vkCreateBuffer(m_device, &vBufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS)
		return false;

	VkMemoryRequirements vbMemoryRequirements;
	vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &vbMemoryRequirements);

	VkMemoryAllocateInfo vbAllocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = vbMemoryRequirements.size,
		.memoryTypeIndex = GetMemoryTypeIndex( &vbMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryProperties),
	};

	if(vkAllocateMemory(m_device, &vbAllocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS)
		return false;

	vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

	// Create Index Buffer
	VkBufferCreateInfo iBufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = sizeof(UInt32) * m_mesh->GetIndexCount(),
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	if (vkCreateBuffer(m_device, &iBufferInfo, nullptr, &m_indexBuffer) != VK_SUCCESS)
		return false;

	VkMemoryRequirements ibMemoryRequirements;
	vkGetBufferMemoryRequirements(m_device, m_indexBuffer, &ibMemoryRequirements);

	VkMemoryAllocateInfo ibAllocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = ibMemoryRequirements.size,
		.memoryTypeIndex = GetMemoryTypeIndex(&ibMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryProperties),
	};

	if (vkAllocateMemory(m_device, &ibAllocInfo, nullptr, &m_indexBufferMemory) != VK_SUCCESS)
		return false;

	vkBindBufferMemory(m_device, m_indexBuffer, m_indexBufferMemory, 0);

	return true;
}

void QuantumEngine::Rendering::Vulkan::VulkanMeshController::CopyCommand(VkCommandBuffer commandBuffer, VkBuffer stageBuffer, UInt32 offset)
{
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = offset;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(Vertex) * m_mesh->GetVertexCount();
	vkCmdCopyBuffer(commandBuffer, stageBuffer, m_vertexBuffer, 1, &copyRegion);
	copyRegion.srcOffset = offset + sizeof(Vertex) * m_mesh->GetVertexCount();
	copyRegion.size = sizeof(UInt32) * m_mesh->GetIndexCount();
	vkCmdCopyBuffer(commandBuffer, stageBuffer, m_indexBuffer, 1, &copyRegion);
}
