// QuantumEngine.Test.cpp : Defines the entry point for the application.
//

#include "QuantumEngine.Test.h"
#include "Platform/Application.h"
#include "Platform/GraphicWindow.h"
#include "DX12GPUDeviceManager.h"
#include "Rendering/GraphicContext.h"
#include "Rendering/Shader.h"
#include "HLSLShader.h"
#include "HLSLShaderImporter.h"
#include "Core/Mesh.h"
#include "Rendering/ShaderProgram.h"
#include "Core/GameEntity.h"
#include "Rendering/GPUAssetManager.h"
#include "HLSLMaterial.h"
#include "Core/Texture2D.h"
#include "Core/WICTexture2DImporter.h"
#include "Core/Matrix4.h"
#include "Core/Transform.h"
#include "Core/Camera/PerspectiveCamera.h"
#include "CameraController.h"
#include "Core/Light/Lights.h"
#include "Core/AssimpModel3DImporter.h"
#include "Core/Model3DAsset.h"
#include "StringUtilities.h"
#include "Core/ShapeBuilder.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/RayTracingComponent.h"
#include "DX12ShaderRegistery.h"
#include "Rendering/GBufferRTReflectionRenderer.h"
#include "FrameRateLogger.h"
#include "Core/Scene.h"
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

    if (SceneBuilder::Run_LightSample_Hybrid(gpuDevice, win, errorStr) == false) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Running the app: \n") + errorStr).c_str(), "App Error", 0);
        return 0;
    }
}
