#include "DemoAPI.h"
#include <Platform/Application.h>
#include <Platform/GraphicWindow.h>
#include <DX12GPUDeviceManager.h>
#include <Rendering/GraphicContext.h>
#include "SceneBuilder.h"
#include <Core/Scene.h>


namespace OS = QuantumEngine::Platform; 
namespace DX12 = QuantumEngine::Rendering::DX12;

bool Run_Light_Scene_Rasterization_DX12(HWND parentWindow)
{
	OS::Application::CreateApplication(GetModuleHandleA(nullptr));
	auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
	auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"First Window", .parentWinHandle = parentWindow });

	auto gpuContext = gpuDevice->CreateHybridContextForWindows(win);
	auto assetManager = gpuDevice->CreateAssetManager();
	gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistery = gpuDevice->CreateShaderRegistery();
	gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = gpuDevice->CreateMaterialFactory();
	std::string error;
	auto scene = SceneBuilder::BuildLightScene(assetManager, shaderRegistery, materialRegistery, win, error);
	
	if (scene == nullptr) {
		//errorStr = error;
		return false;
	}

	gpuContext->PrepareScene(scene);
	Platform::Application::Run(win, gpuContext, scene->behaviours);
	
	return true;
}
