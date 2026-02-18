#include "vulkan-pch.h"
#include "VulkanMeshController.h"
#include "Core/Mesh.h"

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

bool QuantumEngine::Rendering::Vulkan::VulkanMeshController::Initialize(VkPhysicalDevice physicalDevice)
{
	m_physicalDevice = physicalDevice;
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	// Create Vertex Buffer
	VkBufferCreateInfo vBufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = sizeof(Vertex) * m_mesh->GetVertexCount(),
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
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
		.memoryTypeIndex = GetMemoryTypeIndex( &vbMemoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memProperties),
	};

	if(vkAllocateMemory(m_device, &vbAllocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS)
		return false;

	vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

	void* data;
	vkMapMemory(m_device, m_vertexBufferMemory, 0, vBufferInfo.size, 0, &data);
	m_mesh->CopyVertexData((Byte*)data);
	vkUnmapMemory(m_device, m_vertexBufferMemory);

	// Create Index Buffer
	VkBufferCreateInfo iBufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = sizeof(UInt32) * m_mesh->GetIndexCount(),
		.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
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
		.memoryTypeIndex = GetMemoryTypeIndex(&ibMemoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memProperties),
	};

	if (vkAllocateMemory(m_device, &ibAllocInfo, nullptr, &m_indexBufferMemory) != VK_SUCCESS)
		return false;

	vkBindBufferMemory(m_device, m_indexBuffer, m_indexBufferMemory, 0);

	vkMapMemory(m_device, m_indexBufferMemory, 0, iBufferInfo.size, 0, &data);
	m_mesh->CopyIndexData((Byte*)data);
	vkUnmapMemory(m_device, m_indexBufferMemory);

	return true;
}

UInt32 QuantumEngine::Rendering::Vulkan::VulkanMeshController::GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties)
{
	for (UInt32 i = 0; i < memoryProperties->memoryTypeCount; i++) {
		if ((memoryRequirement->memoryTypeBits & (1 << i)) && (memoryProperties->memoryTypes[i].propertyFlags & targetFlags) == targetFlags) {
			return i;
		}
	}
	
	return UINT32_MAX;
}
