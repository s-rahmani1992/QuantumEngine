#pragma once
#include "vulkan-pch.h"

#define ALIGN(x,a) ((x + a - 1) / a) * a
#define HLSL_OBJECT_TRANSFORM_DATA_NAME "_ObjectTransformData"

UInt32 GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties);

bool CreateBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, UInt32 size, UInt32 count, VkBuffer* buffer, VkDeviceMemory* memory);