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

using namespace QuantumEngine;
namespace DX12 = QuantumEngine::Rendering::DX12;

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
    ref<Render::Shader> vertexShader = DX12::HLSLShaderImporter::Import(lightVertexShaderPath, DX12::VERTEX_SHADER, errorStr);

    if (vertexShader == nullptr) {
		error = "Error in Compiling Shader At: \n" + WStringToString(lightVertexShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    ref<Render::Shader> pixelShader = DX12::HLSLShaderImporter::Import(lightPixelShaderPath, DX12::PIXEL_SHADER, errorStr);

    if (pixelShader == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(lightPixelShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Program", { vertexShader, pixelShader }, false);
    
	////// Creating the materials

    ref<DX12::HLSLMaterial> carMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    carMaterial1->Initialize(false);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetFloat("ambient", 0.1f);
	carMaterial1->SetFloat("diffuse", 1.0f);
    carMaterial1->SetFloat("specular", 0.1f);

    ref<DX12::HLSLMaterial> rabbitStatueMaterial1 = std::make_shared<DX12::HLSLMaterial>(lightProgram);
    rabbitStatueMaterial1->Initialize(false);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetFloat("ambient", 0.1f);
    rabbitStatueMaterial1->SetFloat("diffuse", 0.8f);
    rabbitStatueMaterial1->SetFloat("specular", 0.1f);

	////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(0.0f, 3.0f, 1.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, nullptr);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(5.2f, 1.4f, 3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, nullptr);

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
        .radius = 6.0f,
        });
    
    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { carEntity1, rabbitStatueEntity1};
    scene->behaviours = { cameraController, frameLogger };
    
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
