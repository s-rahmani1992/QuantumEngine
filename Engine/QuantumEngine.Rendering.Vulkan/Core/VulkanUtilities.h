#pragma once
#include "vulkan-pch.h"

#define ALIGN(x,a) ((x + a - 1) / a) * a
#define HLSL_OBJECT_TRANSFORM_DATA_NAME "_ObjectTransformData"
#define HLSL_CAMERA_DATA_NAME "_CameraData"

UInt32 GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties);

bool CreateBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, UInt32 size, UInt32 count, VkBuffer* buffer, VkDeviceMemory* memory);