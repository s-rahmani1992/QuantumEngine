// QuantumEngine.Test.cpp : Defines the entry point for the application.
//

#include "framework.h"
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

namespace OS = QuantumEngine::Platform;
namespace DX12 = QuantumEngine::Rendering::DX12;
namespace Render = QuantumEngine::Rendering;

using namespace QuantumEngine;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    OS::Application::CreateApplication(hInstance);
    auto gpuDevice = OS::Application::InitializeGraphicDevice<DX12::DX12GPUDeviceManager>();
    auto win = OS::Application::CreateGraphicWindow({ .width = 1280, .height = 720, .title = L"First Window" });
    auto gpuContext = gpuDevice->CreateHybridContextForWindows(win);
    auto assetManager = gpuDevice->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
	auto shaderRegistry = gpuDevice->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistry);

    LPWSTR rootF = new WCHAR[500];
    DWORD size;
    size = GetModuleFileNameW(NULL, rootF, 500);
    std::wstring root = std::wstring(rootF, size);

    const size_t last_slash_idx = root.rfind('\\');
    if (std::string::npos != last_slash_idx)
        root = root.substr(0, last_slash_idx);

    delete[] rootF;
    std::string errorStr;

    // Importing Textures
    ref<QuantumEngine::Texture2D> tex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\player.png", errorStr);
    ref<QuantumEngine::Texture2D> tex2 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\bg.jpg", errorStr);
    ref<QuantumEngine::Texture2D> skyBoxTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\skybox.jpg", errorStr);
    ref<QuantumEngine::Texture2D> waterTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\water.jpeg", errorStr);
    ref<QuantumEngine::Texture2D> groundTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\ground.jpg", errorStr);
    ref<QuantumEngine::Texture2D> carTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);

    assetManager->UploadTextureToGPU(tex1);
    assetManager->UploadTextureToGPU(tex2);
    assetManager->UploadTextureToGPU(skyBoxTex);
    assetManager->UploadTextureToGPU(waterTex);
    assetManager->UploadTextureToGPU(groundTex);
    assetManager->UploadTextureToGPU(carTex);

    // Creating the camera
    auto camtransform = std::make_shared<Transform>(Vector3(0.0f, 5.0f, -3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 20);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    // Compiling Shaders
    ref<Render::Shader> vertexShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\color.vert.hlsl", DX12::VERTEX_SHADER, errorStr);

    if (vertexShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> pixelShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\color.pix.hlsl", DX12::PIXEL_SHADER, errorStr);

    if (pixelShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> rtShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\rt_shader.lib.hlsl", DX12::LIB_SHADER, errorStr);

    if (rtShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> rtColorShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\rt_color.lib.hlsl", DX12::LIB_SHADER, errorStr);
    if (rtColorShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> rtSolidColorShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\rt_solid_color.lib.hlsl", DX12::LIB_SHADER, errorStr);
    if (rtSolidColorShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> rtWaterShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\rt_water.lib.hlsl", DX12::LIB_SHADER, errorStr);
    if (rtWaterShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> rtGroundShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\rt_ground.lib.hlsl", DX12::LIB_SHADER, errorStr);
    if (rtGroundShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> rtRefractorShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\rt_refractor.lib.hlsl", DX12::LIB_SHADER, errorStr);
    if (rtRefractorShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> gBufferMirrorVertexShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\g_buffer_mirror.vert.hlsl", DX12::VERTEX_SHADER, errorStr);

    if (gBufferMirrorVertexShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    ref<Render::Shader> gBufferMirrorPixelShader = DX12::HLSLShaderImporter::Import(root + L"\\Assets\\Shaders\\g_buffer_mirror.pix.hlsl", DX12::PIXEL_SHADER, errorStr);

    if (gBufferMirrorPixelShader == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Compiling Shader: \n") + errorStr).c_str(), "Shader Compile Error", 0);
        return 0;
    }

    auto program = shaderRegistry->CreateAndRegisterShaderProgram("Simple_Program", { vertexShader, pixelShader }, false);
    auto rtProgram = shaderRegistry->CreateAndRegisterShaderProgram("Simple_RT_Program", { rtShader }, false);
    auto rtColorProgram = shaderRegistry->CreateAndRegisterShaderProgram("Simple_RT_Color_Program", { rtColorShader }, true);
    auto rtSolidColorProgram = shaderRegistry->CreateAndRegisterShaderProgram("Simple_RT_Solid_Color_Program", { rtSolidColorShader }, true);
    auto rtWaterProgram = shaderRegistry->CreateAndRegisterShaderProgram("Simple_RT_Reflection_Program", { rtWaterShader }, true);
    auto rtGroundProgram = shaderRegistry->CreateAndRegisterShaderProgram("Simple_RT_Ground_Program", { rtGroundShader }, true);
    auto rtRefractorProgram = shaderRegistry->CreateAndRegisterShaderProgram("Simple_RT_Refraction_Program", { rtRefractorShader }, true);
	auto gBufferMirrorProgram = shaderRegistry->CreateAndRegisterShaderProgram("G_Buffer_Mirror_Program", { gBufferMirrorVertexShader, gBufferMirrorPixelShader }, false);

    auto model = AssimpModel3DImporter::Import(WCharToString((root + L"\\Assets\\Models\\RetroCar.fbx").c_str()), ModelImportProperties{.axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f)}, errorStr);
    if(model == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Importing Model: \n") + errorStr).c_str(), "Model Import Error", 0);
        return 0;
	}

	auto carMesh = model->GetMesh("Cube.002");

    auto lionModel = AssimpModel3DImporter::Import(WCharToString((root + L"\\Assets\\Models\\lion-lp.fbx").c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, errorStr);
    if (lionModel == nullptr) {
        MessageBoxA(win->GetHandle(), (std::string("Error in Importing Model: \n") + errorStr).c_str(), "Model Import Error", 0);
        return 0;
    }

	auto lionMesh = lionModel->GetMesh("Model.004");

    // Adding Meshes
    std::vector<Vertex> pyramidVertices = {
        Vertex(Vector3(-1.0f, 0.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(-1.0f, -2.0f, -1.0f).Normalize()),
        Vertex(Vector3(1.0f, 0.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(1.0f, -2.0f, -1.0f).Normalize()),
        Vertex(Vector3(1.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(1.0f, -2.0f, 1.0f).Normalize()),
        Vertex(Vector3(-1.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(-1.0f, -2.0f, 1.0f).Normalize()),
        Vertex(Vector3(0.0f, 2.0f, 0.0f), Vector2(0.5f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> pyramidIndices = {
        1, 4, 0,
        2, 4, 1,
        3, 4, 2,
        0, 4, 3,
        0, 3, 2,
        0, 2, 1,
    };

    std::shared_ptr<Mesh> pyramidMesh = std::make_shared<Mesh>(pyramidVertices, pyramidIndices);

    std::shared_ptr<Mesh> cubeMesh = ShapeBuilder::CreateCompleteCube(1.0f);
    std::shared_ptr<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 50, 50);

    std::vector<Vertex> skyBoxVertices = {
        Vertex(Vector3(-1.0f, -1.0f, -1.0f), Vector2(1.0f, 2.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(1.0f, -1.0f, -1.0f), Vector2(0.75f, 2.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(1.0f, 1.0f, -1.0f), Vector2(0.75f, 1.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(-1.0f, 1.0f, -1.0f), Vector2(1.0f, 1.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.25f, 2.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(1.0f, -1.0f, 1.0f), Vector2(0.5f, 2.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(1.0f, 1.0f, 1.0f), Vector2(0.5f, 1.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(-1.0f, 1.0f, 1.0f), Vector2(0.25f, 1.0f / 3), Vector3(0.0f)),
        Vertex(Vector3(-1.0f, 1.0f, -1.0f), Vector2(0.25f, 0.0f), Vector3(0.0f)),
        Vertex(Vector3(1.0f, 1.0f, -1.0f), Vector2(0.5f, 0.0f), Vector3(0.0f)),
        Vertex(Vector3(1.0f, -1.0f, -1.0f), Vector2(0.25f, 1.0f), Vector3(0.0f)),
        Vertex(Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.5f, 1.0f), Vector3(0.0f)),
    };

    std::vector<UInt32> skyBoxIndices = {
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        2, 6, 5, 2, 5, 1,
        0, 4, 7, 0, 7, 3,
        6, 8, 7, 6, 9, 8,
        4, 10, 5, 4, 11, 10,
    };

    std::shared_ptr<Mesh> skyBoxMesh = std::make_shared<Mesh>(skyBoxVertices, skyBoxIndices);

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    std::shared_ptr<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

	assetManager->UploadMeshesToGPU({ carMesh, lionMesh, cubeMesh, sphereMesh, pyramidMesh, skyBoxMesh, planeMesh });
    
    Matrix4 project = mainCamera->ProjectionMatrix();

    ref<DX12::HLSLMaterial> material1 = std::make_shared<DX12::HLSLMaterial>(program);
    material1->Initialize(false);
    material1->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    material1->SetTexture2D("mainTexture", tex2);

    ref<DX12::HLSLMaterial> rtMaterial1 = std::make_shared<DX12::HLSLMaterial>(rtColorProgram);
    rtMaterial1->Initialize(true);
    rtMaterial1->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    rtMaterial1->SetTexture2D("mainTexture", tex2);
    rtMaterial1->SetUInt32("castShadow", 1);

    ref<DX12::HLSLMaterial> material2 = std::make_shared<DX12::HLSLMaterial>(program);
    material2->Initialize(false);
    material2->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    material2->SetTexture2D("mainTexture", tex2);

    ref<DX12::HLSLMaterial> rtMaterial2 = std::make_shared<DX12::HLSLMaterial>(rtColorProgram);
    rtMaterial2->Initialize(true);
    rtMaterial2->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    rtMaterial2->SetTexture2D("mainTexture", tex2);
    rtMaterial2->SetUInt32("castShadow", 1);

    ref<DX12::HLSLMaterial> material3 = std::make_shared<DX12::HLSLMaterial>(program);
    material3->Initialize(false);
    material3->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    material3->SetTexture2D("mainTexture", carTex);

    ref<DX12::HLSLMaterial> rtMaterial3 = std::make_shared<DX12::HLSLMaterial>(rtColorProgram);
    rtMaterial3->Initialize(true);
    rtMaterial3->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    rtMaterial3->SetTexture2D("mainTexture", carTex);
    rtMaterial3->SetUInt32("castShadow", 1);

    ref<DX12::HLSLMaterial> skyboxMaterial = std::make_shared<DX12::HLSLMaterial>(program);
    skyboxMaterial->Initialize(false);
    skyboxMaterial->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    skyboxMaterial->SetTexture2D("mainTexture", skyBoxTex);

    ref<DX12::HLSLMaterial> skyboxRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtColorProgram);
    skyboxRTMaterial->Initialize(true);
    skyboxRTMaterial->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    skyboxRTMaterial->SetTexture2D("mainTexture", skyBoxTex);
    skyboxRTMaterial->SetUInt32("castShadow", 0);

    ref<DX12::HLSLMaterial> planeMaterial = std::make_shared<DX12::HLSLMaterial>(program);
    planeMaterial->Initialize(false);
    planeMaterial->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    planeMaterial->SetTexture2D("mainTexture", groundTex);

    ref<DX12::HLSLMaterial> planeRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtGroundProgram);
    planeRTMaterial->Initialize(true);
    planeRTMaterial->SetColor("color", Color(0.1f, 0.7f, 1.0f, 1.0f));
    planeRTMaterial->SetTexture2D("mainTexture", groundTex);

    ref<DX12::HLSLMaterial> mirrorMaterial = std::make_shared<DX12::HLSLMaterial>(program);
    mirrorMaterial->Initialize(false);
    mirrorMaterial->SetColor("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    mirrorMaterial->SetTexture2D("mainTexture", waterTex);

    ref<DX12::HLSLMaterial> mirrorRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtWaterProgram);
    mirrorRTMaterial->Initialize(true);
    mirrorRTMaterial->SetColor("color", Color(0.1f, 0.7f, 1.0f, 1.0f));
    mirrorRTMaterial->SetTexture2D("mainTexture", waterTex);
    mirrorRTMaterial->SetUInt32("castShadow", 0);

    ref<DX12::HLSLMaterial> refractorRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtRefractorProgram);
    refractorRTMaterial->Initialize(true);
    refractorRTMaterial->SetColor("color", Color(0.1f, 0.7f, 1.0f, 1.0f));
    refractorRTMaterial->SetFloat("refractionFactor", 1.2f);
    refractorRTMaterial->SetUInt32("castShadow", 0);

	ref<DX12::HLSLMaterial> mirrorGBufferMaterial = std::make_shared<DX12::HLSLMaterial>(gBufferMirrorProgram);
	mirrorGBufferMaterial->Initialize(false);
	mirrorGBufferMaterial->SetTexture2D("mainTexture", waterTex);

    auto transform1 = std::make_shared<Transform>(Vector3(0.0f, 3.0f, 1.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
	auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(pyramidMesh, material1);
	auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(pyramidMesh, rtMaterial1);
    auto entity1 = std::make_shared<QuantumEngine::GameEntity>(transform1, meshRenderer1, rtComponent1);
    
    auto transform2 = std::make_shared<Transform>(Vector3(5.2f, 3.4f, 3.0f), Vector3(0.6f), Vector3(0.0f, 1.0f, 1.0f), 120);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(carMesh, material3);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(carMesh, rtMaterial3);
    auto entity2 = std::make_shared<QuantumEngine::GameEntity>(transform2, meshRenderer2, rtComponent2);
    
    auto transform3 = std::make_shared<Transform>(Vector3(2.2f, 3.0f, 4.0f), Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
	auto mirrorRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(lionMesh, mirrorGBufferMaterial);
	auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(lionMesh, mirrorRTMaterial);
    auto entity3 = std::make_shared<QuantumEngine::GameEntity>(transform3, mirrorRenderer, rtComponent3);
    /*auto skyBoxTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(40.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto skyBoxEntity = std::make_shared<QuantumEngine::GameEntity>(skyBoxTransform, skyBoxMesh, skyboxMaterial, skyboxRTMaterial);
    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(40.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto groundEntity = std::make_shared<QuantumEngine::GameEntity>(groundTransform, planeMesh, planeMaterial, planeRTMaterial);
    auto mirrorTransform = std::make_shared<Transform>(Vector3(-3.0f, 5.0f, 0.0f), Vector3(5.0f), Vector3(0.0f, 0.0f, 1.0f), 90);
    auto mirrorEntity = std::make_shared<QuantumEngine::GameEntity>(mirrorTransform, planeMesh, mirrorMaterial, mirrorRTMaterial);
    auto transform4 = std::make_shared<Transform>(Vector3(-2.2f, 2.4f, 3.0f), Vector3(1.6f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto entity4 = std::make_shared<QuantumEngine::GameEntity>(transform4, lionMesh, mirrorMaterial, refractorRTMaterial);*/
    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 0.0f),
        .ambient = 0.3f,
        .diffuse = 1.3f,
        .specular = 0.1f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-0.2f, 6.4f, 3.0f),
        .ambient = 0.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 1.0f,
            .c2 = 0.0f,
        },
        .diffuse = 20.2f,
        .specular = 0.3f,
        .radius = 9.0f,
        });

	gpuContext->PrepareScene({ entity1, entity2, entity3 }, mainCamera, lightData, rtProgram);
    
	auto frameLogger = std::make_shared<FrameRateLogger>();
    Platform::Application::Run(win, gpuContext, std::vector<ref<Behaviour>>({ cameraController, frameLogger }));
    // Initialize global strings
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_QUANTUMENGINETEST, szWindowClass, MAX_LOADSTRING);
    //MyRegisterClass(hInstance);

    //// Perform application initialization:
    //if (!InitInstance (hInstance, nCmdShow))
    //{
    //    return FALSE;
    //}

    //HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_QUANTUMENGINETEST));

    //MSG msg;

    //// Main message loop:
    //while (GetMessage(&msg, nullptr, 0, 0))
    //{
    //    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    }
    //}

    //return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_QUANTUMENGINETEST));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_QUANTUMENGINETEST);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
