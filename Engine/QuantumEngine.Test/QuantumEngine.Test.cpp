// QuantumEngine.Test.cpp : Defines the entry point for the application.
//

#include "QuantumEngine.Test.h"
#include "Platform/Application.h"
#include "Platform/GraphicWindow.h"
#include "DX12GPUDeviceManager.h"
#include "SceneBuilder.h"

namespace OS = QuantumEngine::Platform;
namespace DX12 = QuantumEngine::Rendering::DX12;
namespace Render = QuantumEngine::Rendering;

using namespace QuantumEngine;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    OS::Application::CreateApplication(hInstance);
    auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
    auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"First Window" });
    
    std::string errorStr;

    if (SceneBuilder::Run_ComplexScene_RayTracing(gpuDevice, win, errorStr) == false) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + errorStr).c_str(), "App Error", 0);
        return 0;
    }
}
