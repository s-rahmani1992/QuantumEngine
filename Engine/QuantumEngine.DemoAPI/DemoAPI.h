#pragma once
#include <Windows.h>

#define DEMP_API extern "C" __declspec(dllexport) 

DEMP_API bool Run_Light_Scene_Rasterization_DX12(HWND parentWindow);
