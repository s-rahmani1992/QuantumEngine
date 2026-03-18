#pragma once
#include <Windows.h>

//#if defined(DEMP_API_EXPORTS)
#define DEMO_API extern "C" __declspec(dllexport)
//#else
// define DEMO_API extern "C" __declspec(dllimport)
//#endif

DEMO_API bool Run_Light_Scene_Rasterization_DX12(HWND parentWindow);
