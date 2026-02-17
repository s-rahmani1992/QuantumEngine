// vulkan-pch.h: This is a precompiled header file.

#ifndef VULKAN_PCH_H
#define VULKAN_PCH_H

#define VK_USE_PLATFORM_WIN32_KHR
#define BOOST_JSON_NO_LIB
#define BOOST_CONTAINER_NO_LIB
#include <BasicTypes.h>
#include <vulkan/vulkan.h>
#include <dxcapi.h>
#include "Core/spirv_reflect.h"
#include <wrl/client.h>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "dxcompiler.lib")

#endif //VULKAN_PCH_H
