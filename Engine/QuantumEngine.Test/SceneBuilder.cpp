#include "SceneBuilder.h"
#include "StringUtilities.h"
#include "Platform/Application.h"
#include "Platform/GraphicWindow.h"
#include "DX12GPUDeviceManager.h"
#include "Rendering/ShaderRegistery.h"
#include "Rendering/GPUAssetManager.h"
#include "HLSLShader.h"
#include "HLSLShaderImporter.h"
#include "HLSLMaterial.h"
#include <Core/Light/Lights.h>
#include <Core/WICTexture2DImporter.h>
#include <Core/AssimpModel3DImporter.h>
#include <Core/Camera/PerspectiveCamera.h>
#include <Core/Model3DAsset.h>
#include <Core/GameEntity.h>
#include <Rendering/MeshRenderer.h>
#include "CameraController.h"
#include <Core/Transform.h>
#include <Core/Scene.h>
#include <Rendering/GraphicContext.h>
#include "FrameRateLogger.h"
#include <Rendering/RayTracingComponent.h>
#include <Core/ShapeBuilder.h>
#include <Rendering/GBufferRTReflectionRenderer.h>
#include <Core/Mesh.h>

using namespace QuantumEngine;
namespace DX12 = QuantumEngine::Rendering::DX12;

#define IMPORT_SHADER(PATH_VAR, SHADER_VAR, SHADER_TYPE_ENUM, ERROR_VAR)                      \
    ref<Render::Shader> SHADER_VAR = DX12::HLSLShaderImporter::Import(PATH_VAR, SHADER_TYPE_ENUM, errorStr); \
    if ((SHADER_VAR) == nullptr) {                                                               \
        ERROR_VAR = "Error in Compiling Shader At: \n" + WStringToString(PATH_VAR) + "Error: \n" + errorStr; \
        return nullptr;                                                                                 \
    }

ref<Scene> SceneBuilder::BuildLightScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, ref<Platform::GraphicWindow> win, std::string& error)
{
	std::string errorStr;

	std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

	////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(0.0f, 5.0f, -3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

	////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\bg.jpg", errorStr);
    
	assetManager->UploadTextureToGPU(carTex1);
	assetManager->UploadTextureToGPU(rabbitStatueTex1);

	////// Importing meshes

	auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.obj";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    ////// Compiling Shaders

	std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)
    
    std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

    auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);
    
    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    IMPORT_SHADER(rtGlobalShaderPath, rtGlobalShader, DX12::LIB_SHADER, errorStr)
    
	auto rtGlobalProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Global_Program", { rtGlobalShader }, false);

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    IMPORT_SHADER(rtSimpleLightShaderPath, rtSimpleLightShader, DX12::LIB_SHADER, errorStr)
    
	auto rtSimpleLightProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Simple_Light_Program", { rtSimpleLightShader }, true);

    ////// Creating the materials

    ref<DX12::HLSLMaterial> carMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    carMaterial1->Initialize(false);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetFloat("ambient", 0.1f);
	carMaterial1->SetFloat("diffuse", 1.0f);
    carMaterial1->SetFloat("specular", 0.1f);

	ref<DX12::HLSLMaterial> carRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
	carRTMaterial->Initialize(true);
	carRTMaterial->SetTexture2D("mainTexture", carTex1);
	carRTMaterial->SetFloat("ambient", 0.1f);
	carRTMaterial->SetFloat("diffuse", 1.0f);
	carRTMaterial->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> rabbitStatueMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    rabbitStatueMaterial1->Initialize(false);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetFloat("ambient", 0.1f);
    rabbitStatueMaterial1->SetFloat("diffuse", 0.8f);
    rabbitStatueMaterial1->SetFloat("specular", 0.1f);

	ref<DX12::HLSLMaterial> rabbitStatueRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
	rabbitStatueRTMaterial->Initialize(true);
	rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
	rabbitStatueRTMaterial->SetFloat("ambient", 0.1f);
	rabbitStatueRTMaterial->SetFloat("diffuse", 0.8f);
	rabbitStatueRTMaterial->SetFloat("specular", 0.1f);


	////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(0.0f, 3.0f, 1.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
	auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(5.2f, 1.4f, 3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
	auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

	////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
		.intensity = 0.5f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-0.2f, 4.4f, 3.0f),
        .intensity = 2.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 1.0f,
            .c2 = 0.0f,
        },
        .radius = 9.0f,
        });
    
    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { carEntity1, rabbitStatueEntity1};
    scene->behaviours = { cameraController, frameLogger };
	scene->rtGlobalProgram = rtGlobalProgram;
    
    return scene;
}

ref<Scene> SceneBuilder::BuildReflectionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, ref<Platform::GraphicWindow> win, std::string& error)
{
    std::string errorStr;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(0.0f, 5.0f, -3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> waterTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\water.jpeg", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\bg.jpg", errorStr);

    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(waterTex1);

    ////// Importing meshes

    auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.obj";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto sphereMesh = ShapeBuilder::CreateSphere(1.0f, 24, 24);

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)
    
    std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

    auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    IMPORT_SHADER(rtGlobalShaderPath, rtGlobalShader, DX12::LIB_SHADER, errorStr)

    auto rtGlobalProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Global_Program", { rtGlobalShader }, false);

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    IMPORT_SHADER(rtSimpleLightShaderPath, rtSimpleLightShader, DX12::LIB_SHADER, errorStr)

    auto rtSimpleLightProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Simple_Light_Program", { rtSimpleLightShader }, true);

    std::wstring rtReflectionShaderPath = root + L"\\Assets\\Shaders\\reflection_rt.lib.hlsl";
    IMPORT_SHADER(rtReflectionShaderPath, rtReflectionShader, DX12::LIB_SHADER, errorStr)
    
    auto rtReflectionProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Reflection_Program", { rtReflectionShader }, true);

    std::wstring gBufferReflectionVertPath = root + L"\\Assets\\Shaders\\reflection_g_buffer.vert.hlsl";
    IMPORT_SHADER(gBufferReflectionVertPath, gBufferReflectionVertexShader, DX12::VERTEX_SHADER, errorStr)
    
    std::wstring gBufferReflectionPixPath = root + L"\\Assets\\Shaders\\reflection_g_buffer.pix.hlsl";
    IMPORT_SHADER(gBufferReflectionPixPath, gBufferReflectionPixelShader, DX12::PIXEL_SHADER, errorStr)
    
    auto gBufferReflectionProgram = shaderRegistery->CreateAndRegisterShaderProgram("G_Buffer_Reflection_Program", { gBufferReflectionVertexShader, gBufferReflectionPixelShader }, false);

    ////// Creating the materials

    ref<DX12::HLSLMaterial> carMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    carMaterial1->Initialize(false);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetFloat("ambient", 0.1f);
    carMaterial1->SetFloat("diffuse", 1.0f);
    carMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> carRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
    carRTMaterial->Initialize(true);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetFloat("ambient", 0.1f);
    carRTMaterial->SetFloat("diffuse", 1.0f);
    carRTMaterial->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> rabbitStatueMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    rabbitStatueMaterial1->Initialize(false);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetFloat("ambient", 0.1f);
    rabbitStatueMaterial1->SetFloat("diffuse", 0.8f);
    rabbitStatueMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> rabbitStatueRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
    rabbitStatueRTMaterial->Initialize(true);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetFloat("ambient", 0.1f);
    rabbitStatueRTMaterial->SetFloat("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> gBufferReflectionMaterial1 = std::make_shared<DX12::HLSLMaterial>(gBufferReflectionProgram);
    gBufferReflectionMaterial1->Initialize(false);
    gBufferReflectionMaterial1->SetTexture2D("mainTexture", waterTex1);
    gBufferReflectionMaterial1->SetFloat("reflectivity", 0.8f);
    gBufferReflectionMaterial1->SetFloat("ambient", 0.1f);
    gBufferReflectionMaterial1->SetFloat("diffuse", 0.8f);
    gBufferReflectionMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> reflectionRTMaterial1 = std::make_shared<DX12::HLSLMaterial>(rtReflectionProgram);
    reflectionRTMaterial1->Initialize(true);
    reflectionRTMaterial1->SetTexture2D("mainTexture", waterTex1);
    reflectionRTMaterial1->SetFloat("reflectivity", 0.8f);
    reflectionRTMaterial1->SetUInt32("maxRecursion", 1);
    reflectionRTMaterial1->SetFloat("ambient", 0.1f);
    reflectionRTMaterial1->SetFloat("diffuse", 0.8f);
    reflectionRTMaterial1->SetFloat("specular", 0.1f);


    ////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(0.0f, 3.0f, 1.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(5.2f, 1.4f, 3.0f), Vector3(2.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto mirrorTransform = std::make_shared<Transform>(Vector3(2.2f, 0.4f, 6.0f), Vector3(3.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(sphereMesh, gBufferReflectionMaterial1);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(sphereMesh, reflectionRTMaterial1);
    auto mirrorEntity1 = std::make_shared<QuantumEngine::GameEntity>(mirrorTransform, gBufferRenderer, rtComponent3);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 0.5f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-0.2f, 4.4f, 3.0f),
        .intensity = 2.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 1.0f,
            .c2 = 0.0f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { carEntity1, rabbitStatueEntity1, mirrorEntity1 };
    scene->behaviours = { cameraController, frameLogger };
    scene->rtGlobalProgram = rtGlobalProgram;

    return scene;
}

ref<Scene> SceneBuilder::BuildShadowScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(0.0f, 5.0f, -3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> groundTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\ground.jpg", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\bg.jpg", errorStr);

    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(groundTex1);

    ////// Importing meshes

    auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        errorStr = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.obj";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        errorStr = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

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

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

        auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    IMPORT_SHADER(rtGlobalShaderPath, rtGlobalShader, DX12::LIB_SHADER, errorStr)

        auto rtGlobalProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Global_Program", { rtGlobalShader }, false);

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    IMPORT_SHADER(rtSimpleLightShaderPath, rtSimpleLightShader, DX12::LIB_SHADER, errorStr)

    auto rtSimpleLightProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Simple_Light_Program", { rtSimpleLightShader }, true);

    std::wstring rtShadowShaderPath = root + L"\\Assets\\Shaders\\shadow_rt.lib.hlsl";
    IMPORT_SHADER(rtShadowShaderPath, rtShadowShader, DX12::LIB_SHADER, errorStr)

        auto rtShadowProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Shadow_Program", { rtShadowShader }, true);
    
    ////// Creating the materials

    ref<DX12::HLSLMaterial> carMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    carMaterial1->Initialize(false);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetFloat("ambient", 0.1f);
    carMaterial1->SetFloat("diffuse", 1.0f);
    carMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> carRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
    carRTMaterial->Initialize(true);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetFloat("ambient", 0.1f);
    carRTMaterial->SetFloat("diffuse", 1.0f);
    carRTMaterial->SetFloat("specular", 0.1f);
    carRTMaterial->SetUInt32("castShadow", 1);

    ref<DX12::HLSLMaterial> rabbitStatueMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    rabbitStatueMaterial1->Initialize(false);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetFloat("ambient", 0.1f);
    rabbitStatueMaterial1->SetFloat("diffuse", 0.8f);
    rabbitStatueMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> rabbitStatueRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
    rabbitStatueRTMaterial->Initialize(true);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetFloat("ambient", 0.1f);
    rabbitStatueRTMaterial->SetFloat("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetFloat("specular", 0.1f);
    rabbitStatueRTMaterial->SetUInt32("castShadow", 1);

    ref<DX12::HLSLMaterial> groundShadowMaterial1 = std::make_shared<DX12::HLSLMaterial>(rtShadowProgram);
    groundShadowMaterial1->Initialize(true);
    groundShadowMaterial1->SetTexture2D("mainTexture", groundTex1);
    groundShadowMaterial1->SetFloat("ambient", 0.1f);
    groundShadowMaterial1->SetFloat("diffuse", 0.8f);
    groundShadowMaterial1->SetFloat("specular", 0.1f);

    ////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(0.0f, 3.0f, 1.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(5.2f, 1.4f, 3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(planeMesh, nullptr);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundShadowMaterial1);
    auto groundEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform, gBufferRenderer, rtComponent3);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 1.0f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-0.2f, 6.4f, 3.0f),
        .intensity = 20.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 0.0f,
            .c2 = 1.0f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { carEntity1, rabbitStatueEntity1, groundEntity1 };
    scene->behaviours = { cameraController, frameLogger };
    scene->rtGlobalProgram = rtGlobalProgram;

    return scene;
}

ref<Scene> SceneBuilder::BuildRefractionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(0.0f, 5.0f, -3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> waterTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\water.jpeg", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\bg.jpg", errorStr);

    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(waterTex1);

    ////// Importing meshes

    auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        errorStr = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.obj";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        errorStr = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto cubeMesh = ShapeBuilder::CreateCompleteCube(1.0f);

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

        auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    IMPORT_SHADER(rtGlobalShaderPath, rtGlobalShader, DX12::LIB_SHADER, errorStr)

        auto rtGlobalProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Global_Program", { rtGlobalShader }, false);

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    IMPORT_SHADER(rtSimpleLightShaderPath, rtSimpleLightShader, DX12::LIB_SHADER, errorStr)

        auto rtSimpleLightProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Simple_Light_Program", { rtSimpleLightShader }, true);

    std::wstring rtRefractionShaderPath = root + L"\\Assets\\Shaders\\refractor_rt.lib.hlsl";
    IMPORT_SHADER(rtRefractionShaderPath, rtRefractionShader, DX12::LIB_SHADER, errorStr)

        auto rtRefractionProgram = shaderRegistery->CreateAndRegisterShaderProgram("RT_Reflection_Program", { rtRefractionShader }, true);

    ////// Creating the materials

    ref<DX12::HLSLMaterial> carMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    carMaterial1->Initialize(false);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetFloat("ambient", 0.1f);
    carMaterial1->SetFloat("diffuse", 1.0f);
    carMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> carRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
    carRTMaterial->Initialize(true);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetFloat("ambient", 0.1f);
    carRTMaterial->SetFloat("diffuse", 1.0f);
    carRTMaterial->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> rabbitStatueMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    rabbitStatueMaterial1->Initialize(false);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetFloat("ambient", 0.1f);
    rabbitStatueMaterial1->SetFloat("diffuse", 0.8f);
    rabbitStatueMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> rabbitStatueRTMaterial = std::make_shared<DX12::HLSLMaterial>(rtSimpleLightProgram);
    rabbitStatueRTMaterial->Initialize(true);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetFloat("ambient", 0.1f);
    rabbitStatueRTMaterial->SetFloat("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> refractionRTMaterial1 = std::make_shared<DX12::HLSLMaterial>(rtRefractionProgram);
    refractionRTMaterial1->Initialize(true);
    refractionRTMaterial1->SetFloat("refractionFactor", 1.3f);
    refractionRTMaterial1->SetUInt32("maxRecursion", 5);

    ////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(0.0f, 3.0f, 4.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(3.2f, 1.4f, 5.0f), Vector3(2.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto mirrorTransform = std::make_shared<Transform>(Vector3(-2.2f, 0.4f, 1.0f), Vector3(5.0f, 5.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(cubeMesh, carMaterial1);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(cubeMesh, refractionRTMaterial1);
    auto mirrorEntity1 = std::make_shared<QuantumEngine::GameEntity>(mirrorTransform, gBufferRenderer, rtComponent3);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 0.5f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-0.2f, 4.4f, 3.0f),
        .intensity = 2.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 1.0f,
            .c2 = 0.0f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { carEntity1, rabbitStatueEntity1, mirrorEntity1 };
    scene->behaviours = { cameraController, frameLogger };
    scene->rtGlobalProgram = rtGlobalProgram;

    return scene;
}

bool SceneBuilder::Run_LightSample_Hybrid(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateHybridContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);

	std::string error;

    ref<Scene> scene = BuildLightScene(assetManager, shaderRegistery, win, error);

    if(scene == nullptr) {
        errorStr = error;
        return false;
	}

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);
    
    return true;
}

bool SceneBuilder::Run_LightSample_RayTracing(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateRayTracingContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);

    std::string error;

    ref<Scene> scene = BuildLightScene(assetManager, shaderRegistery, win, error);

    if (scene == nullptr) {
        errorStr = error;
        return false;
    }

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);

    return true;
}

bool SceneBuilder::Run_ReflectionSample_RayTracing(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateRayTracingContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);

    std::string error;

    ref<Scene> scene = BuildReflectionScene(assetManager, shaderRegistery, win, error);

    if (scene == nullptr) {
        errorStr = error;
        return false;
    }

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);

    return true;
}

bool SceneBuilder::Run_ReflectionSample_Hybrid(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateHybridContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);

    std::string error;

    ref<Scene> scene = BuildReflectionScene(assetManager, shaderRegistery, win, error);

    if (scene == nullptr) {
        errorStr = error;
        return false;
    }

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);

    return true;
}

bool SceneBuilder::Run_ShadowSample_RayTracing(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateRayTracingContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);

    std::string error;

    ref<Scene> scene = BuildShadowScene(assetManager, shaderRegistery, win, error);

    if (scene == nullptr) {
        errorStr = error;
        return false;
    }

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);

    return true;
}

bool SceneBuilder::Run_RefractionSample_RayTracing(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateRayTracingContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);

    std::string error;

    ref<Scene> scene = BuildRefractionScene(assetManager, shaderRegistery, win, error);

    if (scene == nullptr) {
        errorStr = error;
        return false;
    }

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);

    return true;
}
