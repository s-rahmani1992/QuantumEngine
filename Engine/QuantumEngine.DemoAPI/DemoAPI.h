#pragma once
#include <Windows.h>

#define DEMO_API extern "C" __declspec(dllexport)

DEMO_API bool Run_Light_Scene_Rasterization_DX12(HWND parentWindow);

DEMO_API bool Run_Light_Scene_RayTracing_DX12(HWND parentWindow);
