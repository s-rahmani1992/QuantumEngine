#include "DemoAPI.h"
#include <Platform/Application.h>
#include <Platform/GraphicWindow.h>
#include <DX12GPUDeviceManager.h>
#include <Core/VulkanDeviceManager.h>
#include <Rendering/GraphicContext.h>
#include "SceneBuilder.h"
#include <Core/Scene.h>

namespace OS = QuantumEngine::Platform; 
namespace DX12 = QuantumEngine::Rendering::DX12;
namespace VK = QuantumEngine::Rendering::Vulkan;

bool Run_Light_Scene_Rasterization_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Simple Scene ---- Rasterization ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateHybridContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildSimpleLightScene(assetManager, shaderRegistery, materialRegistery, win, error);
	
	if (scene == nullptr) {
	    MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Light_Scene_Rasterization_VK(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<VK::VulkanDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Simple Scene ---- Rasterization ---- Vulkan", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateHybridContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildSimpleLightScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Light_Scene_RayTracing_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Simple Scene ---- Ray Tracing ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildSimpleLightScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Light_Scene_RayTracing_VK(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<VK::VulkanDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Simple Scene ---- Ray Tracing ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildSimpleLightScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

DEMO_API bool Run_Reflection_Scene_Hybrid_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Reflection Scene ---- Hybrid ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateHybridContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildReflectionScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Reflection_Scene_RayTracing_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Reflection Scene ---- Ray Tracing ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildReflectionScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Shadow_Scene_RayTracing_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Shadow Scene ---- Ray Tracing ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildShadowScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Refraction_Scene_RayTracing_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Refraction Scene ---- Ray Tracing ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildRefractionScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}

bool Run_Complete_Scene_RayTracing_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"Complete Scene ---- Ray Tracing ---- DirectX 12", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateRayTracingContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildComplexScene(assetManager, shaderRegistery, materialRegistery, win, error);

	if (scene == nullptr) {
		MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + error).c_str(), "Render Error Error", 0);
		DestroyWindow(win->GetHandle());
		Platform::Application::Release();
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);

	DestroyWindow(win->GetHandle());
	Platform::Application::Release();

	return true;
}
