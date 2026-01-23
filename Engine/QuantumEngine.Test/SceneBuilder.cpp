#include "SceneBuilder.h"
#include "StringUtilities.h"

#include "Platform/Application.h"
#include "Platform/GraphicWindow.h"

#include "DX12GPUDeviceManager.h"
#include "Core/HLSLShaderProgram.h"
#include "HLSLShader.h"
#include "HLSLShaderImporter.h"
#include "Shader/HLSLRasterizationProgramImporter.h"
#include "RayTracing/HLSLRayTracingProgramImporter.h"

#include "Rendering/ShaderRegistery.h"
#include "Rendering/GPUAssetManager.h"
#include <Rendering/MeshRenderer.h>
#include <Rendering/GraphicContext.h>
#include <Rendering/GBufferRTReflectionRenderer.h>
#include <Rendering/SplineRenderer.h>
#include <Rendering/RayTracingComponent.h>
#include "Rendering/MaterialFactory.h"

#include <Core/Light/Lights.h>
#include <Core/WICTexture2DImporter.h>
#include <Core/AssimpModel3DImporter.h>
#include <Core/Camera/PerspectiveCamera.h>
#include <Core/Model3DAsset.h>
#include <Core/GameEntity.h>
#include <Core/ShapeBuilder.h>
#include <Core/Mesh.h>
#include <Core/Transform.h>
#include <Core/Scene.h>

#include "CameraController.h"
#include "FrameRateLogger.h"
#include "EntityMover.h"
#include "EntityRotator.h"
#include "TextureAnimator.h"
#include "CurveModifier.h"

using namespace QuantumEngine;
namespace DX12 = QuantumEngine::Rendering::DX12;

#define IMPORT_SHADER(PATH_VAR, SHADER_VAR, SHADER_TYPE_ENUM, ERROR_VAR)                      \
    ref<Render::Shader> SHADER_VAR = DX12::HLSLShaderImporter::Import(PATH_VAR, SHADER_TYPE_ENUM, errorStr); \
    if ((SHADER_VAR) == nullptr) {                                                               \
        ERROR_VAR = "Error in Compiling Shader At: \n" + WStringToString(PATH_VAR) + "Error: \n" + errorStr; \
        return nullptr;                                                                                 \
    }

ref<Scene> SceneBuilder::BuildLightScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& error)
{
	std::string errorStr;

	std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

        auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    
    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    
    std::wstring rtSimpleLightRasterPath = root + L"\\Assets\\Shaders\\simple_light_raster_program.hlsl";
	DX12::Shader::HLSLRasterizationProgramImportDesc lightRasterDesc;
	lightRasterDesc.shaderModel = "6_6";
	lightRasterDesc.vertexMainFunction = "vs_main";
	lightRasterDesc.pixelMainFunction = "ps_main";
	auto lightRasterProgram = DX12::Shader::HLSLRasterizationProgramImporter::ImportShader(rtSimpleLightRasterPath, lightRasterDesc, errorStr);

    if(lightRasterProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightRasterPath) + "Error: \n" + errorStr;
        return nullptr;
	}

	shaderRegistery->RegisterShaderProgram("RT_Simple_Light_Raster_Program", lightRasterProgram, false);
	
	std::wstring curveRasterPath = root + L"\\Assets\\Shaders\\curve_raster_program.hlsl";
    DX12::Shader::HLSLRasterizationProgramImportDesc curveRasterDesc;
    curveRasterDesc.shaderModel = "6_6";
    curveRasterDesc.vertexMainFunction = "vs_main";
    curveRasterDesc.pixelMainFunction = "ps_main";
	curveRasterDesc.geometryMainFunction = "gs_main";
    auto curveRasterProgram = DX12::Shader::HLSLRasterizationProgramImporter::ImportShader(curveRasterPath, curveRasterDesc, errorStr);

    if (curveRasterProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(curveRasterPath) + "Error: \n" + errorStr;
        return nullptr;
    }

	shaderRegistery->RegisterShaderProgram("Curve_Raster_Program", curveRasterProgram, false);

	DX12::RayTracing::HLSLRayTracingProgramProperties simpleRTDesc;
	simpleRTDesc.shaderModel = "6_6";
	simpleRTDesc.rayGenerationFunction = "rayGen";
	simpleRTDesc.missFunction = "miss";
    simpleRTDesc.closestHitFunction = "chs";

	auto globalRTProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtGlobalShaderPath, simpleRTDesc, errorStr);

    if (globalRTProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

	shaderRegistery->RegisterShaderProgram("Global_RT_Program", globalRTProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties simpleLocalRTDesc;
    simpleLocalRTDesc.shaderModel = "6_6";
    simpleLocalRTDesc.closestHitFunction = "chs";

	auto simpleRTLightProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtSimpleLightShaderPath, simpleLocalRTDesc, errorStr);

    if(simpleRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightShaderPath) + "Error: \n" + errorStr;
        return nullptr;
	}

	shaderRegistery->RegisterShaderProgram("Simple_Light_RT_Program", simpleRTLightProgram, true);

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(-6.0f, 2.6f, 1.8f), Vector3(1.0f), Vector3(0.0f, -0.85f, 0.12f), 111);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

	////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    ref<QuantumEngine::Texture2D> lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    ref<QuantumEngine::Texture2D> chairTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\leather_chair_BaseColor.png", errorStr);
    ref<QuantumEngine::Texture2D> groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    assetManager->UploadTextureToGPU(carTex1);
	assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(chairTex1);

	////// Importing meshes

	auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto lionStatuePath = root + L"\\Assets\\Models\\lion-lp.fbx";
    auto lionStatueModel1 = AssimpModel3DImporter::Import(WCharToString(lionStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, errorStr);
    
    if (lionStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(lionStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }    
    
    auto lionStatueMesh1 = lionStatueModel1->GetMesh("Model.004");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{.axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f)}, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

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

    ////// Creating the materials

	auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
	rtGlobalMaterial->SetValue("missColor", Color(0.2f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto carMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    carMaterial1->SetValue("ambient", 0.1f);
    carMaterial1->SetValue("diffuse", 0.5f);
    carMaterial1->SetValue("specular", 2.1f);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetTexture2D("mainTexture1", groundBrickTex1);

	auto carRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
	carRTMaterial->SetTexture2D("mainTexture", carTex1);
	carRTMaterial->SetValue("ambient", 0.1f);
	carRTMaterial->SetValue("diffuse", 1.0f);
	carRTMaterial->SetValue("specular", 0.1f);

    auto rabbitStatueMaterial1 = materialFactory->CreateMaterial(lightProgram);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetValue("ambient", 0.1f);
    rabbitStatueMaterial1->SetValue("diffuse", 0.8f);
    rabbitStatueMaterial1->SetValue("specular", 0.1f);

	auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
	rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
	rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
	rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
	rabbitStatueRTMaterial->SetValue("specular", 0.1f);

    auto lionStatueMaterial1 = materialFactory->CreateMaterial(lightProgram);
    lionStatueMaterial1->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueMaterial1->SetValue("ambient", 0.1f);
    lionStatueMaterial1->SetValue("diffuse", 0.8f);
    lionStatueMaterial1->SetValue("specular", 0.1f);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);

    auto chairMaterial1 = materialFactory->CreateMaterial(lightProgram);
    chairMaterial1->SetTexture2D("mainTexture", chairTex1);
    chairMaterial1->SetValue("ambient", 0.1f);
    chairMaterial1->SetValue("diffuse", 0.8f);
    chairMaterial1->SetValue("specular", 0.1f);

    auto chairRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    chairRTMaterial->SetTexture2D("mainTexture", chairTex1);
    chairRTMaterial->SetValue("ambient", 0.1f);
    chairRTMaterial->SetValue("diffuse", 0.8f);
    chairRTMaterial->SetValue("specular", 0.3f);

    auto groundMaterial1 = materialFactory->CreateMaterial(lightProgram);
    groundMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundMaterial1->SetValue("ambient", 0.1f);
    groundMaterial1->SetValue("diffuse", 0.8f);
    groundMaterial1->SetValue("specular", 0.4f);

    auto groundRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    groundRTMaterial->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial->SetValue("ambient", 0.1f);
    groundRTMaterial->SetValue("diffuse", 0.8f);
    groundRTMaterial->SetValue("specular", 0.4f);

	auto curveMaterial = materialFactory->CreateMaterial(curveRasterProgram);
    curveMaterial->SetValue("ambient", 0.1f);
    curveMaterial->SetValue("diffuse", 0.8f);
    curveMaterial->SetValue("specular", 0.4f);
	curveMaterial->SetValue("color", Color(0.5f, 0.2f, 0.6f, 1.0f));

	////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(-2.0f, 0.5f, -2.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
	auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(2.0f, 0.0f, -2.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), -90);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
	auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(0.0f, 1.5f, 2.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer3 = std::make_shared<Render::MeshRenderer>(lionStatueMesh1, lionStatueMaterial1);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(lionStatueMesh1, lionStatueRTMaterial);
    auto lionStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, meshRenderer3, rtComponent3);

    auto groundTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer4 = std::make_shared<Render::MeshRenderer>(planeMesh, groundMaterial1);
    auto rtComponent4 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial);
    auto grountEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform1, meshRenderer4, rtComponent4);

    auto chairTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, -2.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer5 = std::make_shared<Render::MeshRenderer>(chairMesh1, chairMaterial1);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity1 = std::make_shared<QuantumEngine::GameEntity>(chairTransform1, meshRenderer5, rtComponent5);

    auto chairTransform2 = std::make_shared<Transform>(Vector3(3.0f, 2.0f, -2.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 20);
    auto meshRenderer6 = std::make_shared<Render::MeshRenderer>(chairMesh1, chairMaterial1);
    auto rtComponent6 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity2 = std::make_shared<QuantumEngine::GameEntity>(chairTransform2, meshRenderer6, rtComponent6);

	auto curveTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
	auto curveRenderer = std::make_shared<Render::SplineRenderer>(curveMaterial, std::vector<Vector3>{ Vector3(-4.0f, 0.0f, -4.0f), Vector3(0.0f, 4.0f, 0.0f), Vector3(4.0f, 0.0f, 4.0f) }, 2.2f, 10);
	auto curveEntity = std::make_shared<QuantumEngine::GameEntity>(curveTransform, curveRenderer, nullptr);
    
    auto curveTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
    auto curveRenderer1 = std::make_shared<Render::SplineRenderer>(curveMaterial, std::vector<Vector3>{ Vector3(-4.0f, 0.0f, 0.0f), Vector3(0.0f, 12.0f, 0.0f), Vector3(4.0f, 0.0f, 0.0f) }, 0.8f, 20);
    auto curveEntity1 = std::make_shared<QuantumEngine::GameEntity>(curveTransform1, curveRenderer1, nullptr);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
		.intensity = 0.1f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(0.2f, 4.4f, 0.0f),
        .intensity = 4.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 0.5f,
            .c2 = 0.5f,
        },
        .radius = 9.0f,
        });
    
    auto frameLogger = std::make_shared<FrameRateLogger>();
	auto textureAnimator = std::make_shared<TextureAnimator>(carMaterial1, carTex1, lionStatueTex1, 1.0f);
	auto curveModifier = std::make_shared<CurveModifier>(curveRenderer, 2.0f);
    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { carEntity1, rabbitStatueEntity1, lionStatueEntity1, grountEntity1, chairEntity1, chairEntity2, curveEntity, curveEntity1};
    scene->behaviours = { cameraController, curveModifier };
	scene->rtGlobalMaterial = rtGlobalMaterial;
    
    return scene;
}

ref<Scene> SceneBuilder::BuildReflectionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& error)
{
    std::string errorStr;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

        auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    
    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    
    std::wstring rtReflectionShaderPath = root + L"\\Assets\\Shaders\\reflection_rt.lib.hlsl";
    
    std::wstring gBufferReflectionVertPath = root + L"\\Assets\\Shaders\\reflection_g_buffer.vert.hlsl";
    IMPORT_SHADER(gBufferReflectionVertPath, gBufferReflectionVertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring gBufferReflectionPixPath = root + L"\\Assets\\Shaders\\reflection_g_buffer.pix.hlsl";
    IMPORT_SHADER(gBufferReflectionPixPath, gBufferReflectionPixelShader, DX12::PIXEL_SHADER, errorStr)

        auto gBufferReflectionProgram = shaderRegistery->CreateAndRegisterShaderProgram("G_Buffer_Reflection_Program", { gBufferReflectionVertexShader, gBufferReflectionPixelShader }, false);

    DX12::RayTracing::HLSLRayTracingProgramProperties simpleRTDesc;
    simpleRTDesc.shaderModel = "6_6";
    simpleRTDesc.rayGenerationFunction = "rayGen";
    simpleRTDesc.missFunction = "miss";
    simpleRTDesc.closestHitFunction = "chs";

    auto globalRTProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtGlobalShaderPath, simpleRTDesc, errorStr);

    if (globalRTProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Global_RT_Program", globalRTProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties simpleLocalRTDesc;
    simpleLocalRTDesc.shaderModel = "6_6";
    simpleLocalRTDesc.closestHitFunction = "chs";

    auto simpleRTLightProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtSimpleLightShaderPath, simpleLocalRTDesc, errorStr);

    if (simpleRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Simple_Light_RT_Program", simpleRTLightProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties reflectionLocalRTDesc;
    reflectionLocalRTDesc.shaderModel = "6_6";
    reflectionLocalRTDesc.closestHitFunction = "chs";
    reflectionLocalRTDesc.missFunction = "miss";

    auto reflectionRTLightProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtReflectionShaderPath, reflectionLocalRTDesc, errorStr);

    if (reflectionRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtReflectionShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Simple_Reflection_RT_Program", reflectionRTLightProgram, true);

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(-8.0f, 6.4f, -4.1f), Vector3(1.0f), Vector3(-0.27f, -0.69f, 0.08f), 60);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    ref<QuantumEngine::Texture2D> lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    ref<QuantumEngine::Texture2D> chairTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\leather_chair_BaseColor.png", errorStr);
    ref<QuantumEngine::Texture2D> groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    ref<QuantumEngine::Texture2D> waterTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\water.jpeg", errorStr);
    ref<QuantumEngine::Texture2D> swampTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\swampTex.jpg", errorStr);
    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(chairTex1);
    assetManager->UploadTextureToGPU(waterTex1);
    assetManager->UploadTextureToGPU(swampTex1);

    ////// Importing meshes

    auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto lionStatuePath = root + L"\\Assets\\Models\\lion-lp.fbx";
    auto lionStatueModel1 = AssimpModel3DImporter::Import(WCharToString(lionStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, errorStr);

    if (lionStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(lionStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto lionStatueMesh1 = lionStatueModel1->GetMesh("Model.004");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f) }, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    ref<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

    ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.9f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto carMaterial1 = materialFactory->CreateMaterial(lightProgram);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetValue("ambient", 0.1f);
    carMaterial1->SetValue("diffuse", 1.0f);
    carMaterial1->SetValue("specular", 0.1f);

    auto carRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetValue("ambient", 0.1f);
    carRTMaterial->SetValue("diffuse", 1.0f);
    carRTMaterial->SetValue("specular", 0.1f);

    auto rabbitStatueMaterial1 = materialFactory->CreateMaterial(lightProgram);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetValue("ambient", 0.1f);
    rabbitStatueMaterial1->SetValue("diffuse", 0.8f);
    rabbitStatueMaterial1->SetValue("specular", 0.1f);

    auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
    rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetValue("specular", 0.1f);

    auto lionStatueMaterial1 = materialFactory->CreateMaterial(lightProgram);
    lionStatueMaterial1->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueMaterial1->SetValue("ambient", 0.1f);
    lionStatueMaterial1->SetValue("diffuse", 0.8f);
    lionStatueMaterial1->SetValue("specular", 0.1f);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);

    auto chairMaterial1 = materialFactory->CreateMaterial(lightProgram);
    chairMaterial1->SetTexture2D("mainTexture", chairTex1);
    chairMaterial1->SetValue("ambient", 0.1f);
    chairMaterial1->SetValue("diffuse", 0.8f);
    chairMaterial1->SetValue("specular", 0.1f);

    auto chairRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    chairRTMaterial->SetTexture2D("mainTexture", chairTex1);
    chairRTMaterial->SetValue("ambient", 0.1f);
    chairRTMaterial->SetValue("diffuse", 0.8f);
    chairRTMaterial->SetValue("specular", 0.3f);

    auto groundMaterial1 = materialFactory->CreateMaterial(lightProgram);
    groundMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundMaterial1->SetValue("ambient", 0.1f);
    groundMaterial1->SetValue("diffuse", 0.8f);
    groundMaterial1->SetValue("specular", 0.4f);

    auto groundRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    groundRTMaterial->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial->SetValue("ambient", 0.1f);
    groundRTMaterial->SetValue("diffuse", 0.8f);
    groundRTMaterial->SetValue("specular", 0.4f);

    auto gBufferReflectionMaterial1 = materialFactory->CreateMaterial(gBufferReflectionProgram);
    gBufferReflectionMaterial1->SetTexture2D("mainTexture", swampTex1);
    gBufferReflectionMaterial1->SetValue("reflectivity", 0.8f);
    gBufferReflectionMaterial1->SetValue("ambient", 0.1f);
    gBufferReflectionMaterial1->SetValue("diffuse", 0.8f);
    gBufferReflectionMaterial1->SetValue("specular", 0.1f);

    auto reflectionRTMaterial1 = materialFactory->CreateMaterial(reflectionRTLightProgram);
    reflectionRTMaterial1->SetTexture2D("mainTexture", swampTex1);
    reflectionRTMaterial1->SetValue("castShadow", 0);
    reflectionRTMaterial1->SetValue("reflectivity", 0.5f);
    reflectionRTMaterial1->SetValue("ambient", 0.1f);
    reflectionRTMaterial1->SetValue("diffuse", 0.8f);
    reflectionRTMaterial1->SetValue("specular", 0.8f);

    auto gBufferReflectionMaterial2 = materialFactory->CreateMaterial(gBufferReflectionProgram); 
    gBufferReflectionMaterial2->SetTexture2D("mainTexture", waterTex1);
    gBufferReflectionMaterial2->SetValue("reflectivity", 0.8f);
    gBufferReflectionMaterial2->SetValue("ambient", 0.1f);
    gBufferReflectionMaterial2->SetValue("diffuse", 0.9f);
    gBufferReflectionMaterial2->SetValue("specular", 0.6f);

    auto reflectionRTMaterial2 = materialFactory->CreateMaterial(reflectionRTLightProgram);
    reflectionRTMaterial2->SetTexture2D("mainTexture", waterTex1);
    reflectionRTMaterial2->SetValue("reflectivity", 0.8f);
    reflectionRTMaterial2->SetValue("ambient", 0.1f);
    reflectionRTMaterial2->SetValue("diffuse", 0.9f);
    reflectionRTMaterial2->SetValue("specular", 0.6f);


    ////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(-4.0f, 0.5f, 0.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), -90);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(3.0f, 1.5f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer3 = std::make_shared<Render::MeshRenderer>(lionStatueMesh1, lionStatueMaterial1);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(lionStatueMesh1, lionStatueRTMaterial);
    auto lionStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, meshRenderer3, rtComponent3);

    auto groundTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer4 = std::make_shared<Render::MeshRenderer>(planeMesh, groundMaterial1);
    auto rtComponent4 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial);
    auto grountEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform1, meshRenderer4, rtComponent4);

    auto chairTransform1 = std::make_shared<Transform>(Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer5 = std::make_shared<Render::MeshRenderer>(chairMesh1, chairMaterial1);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity1 = std::make_shared<QuantumEngine::GameEntity>(chairTransform1, meshRenderer5, rtComponent5);

    auto mirrorTransform = std::make_shared<Transform>(Vector3(5.2f, 2.8f, -3.0f), Vector3(2.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(sphereMesh, gBufferReflectionMaterial1);
    auto rtComponent6 = std::make_shared<Render::RayTracingComponent>(sphereMesh, reflectionRTMaterial1);
    auto mirrorEntity1 = std::make_shared<QuantumEngine::GameEntity>(mirrorTransform, gBufferRenderer, rtComponent6);

    auto mirrorTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 3.0f), Vector3(15.0f), Vector3(1.0f, 0.0f, 0.0f), 90);
    auto gBufferRenderer1 = std::make_shared<Render::GBufferRTReflectionRenderer>(planeMesh, gBufferReflectionMaterial2);
    auto rtComponent7 = std::make_shared<Render::RayTracingComponent>(planeMesh, reflectionRTMaterial2);
    auto mirrorEntity2 = std::make_shared<QuantumEngine::GameEntity>(mirrorTransform1, gBufferRenderer1, rtComponent7);


    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 0.5f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(1.0f, 4.4f, 1.0f),
        .intensity = 3.0f,
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
    scene->entities = {carEntity1, rabbitStatueEntity1, lionStatueEntity1, chairEntity1, grountEntity1, mirrorEntity1, mirrorEntity2 };
    scene->behaviours = { cameraController};
    scene->rtGlobalMaterial = rtGlobalMaterial;
    return scene;
}

ref<Scene> SceneBuilder::BuildShadowScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

        auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    
    DX12::RayTracing::HLSLRayTracingProgramProperties simpleRTDesc;
    simpleRTDesc.shaderModel = "6_6";
    simpleRTDesc.rayGenerationFunction = "rayGen";
    simpleRTDesc.missFunction = "miss";
    simpleRTDesc.closestHitFunction = "chs";

    auto globalRTProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtGlobalShaderPath, simpleRTDesc, errorStr);

    if (globalRTProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Global_RT_Program", globalRTProgram, true);

    std::wstring rtShadowShaderPath = root + L"\\Assets\\Shaders\\shadow_rt.lib.hlsl";

    DX12::RayTracing::HLSLRayTracingProgramProperties shadowRTDesc;
    shadowRTDesc.shaderModel = "6_6";
    shadowRTDesc.closestHitFunction = "chs";
    shadowRTDesc.missFunction = "miss";

    auto rtShadowProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtShadowShaderPath, shadowRTDesc, errorStr);

    if (rtShadowProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtShadowShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("RT_Shadow_Program", rtShadowProgram, true);
    
    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(4.85f, 5.4f, 7.7f), Vector3(1.0f), Vector3(0.0f, 0.21f, -0.05f), 151);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    ref<QuantumEngine::Texture2D> lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    ref<QuantumEngine::Texture2D> chairTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\leather_chair_BaseColor.png", errorStr);
    ref<QuantumEngine::Texture2D> groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(chairTex1);

    ////// Importing meshes

    auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto lionStatuePath = root + L"\\Assets\\Models\\lion-lp.fbx";
    auto lionStatueModel1 = AssimpModel3DImporter::Import(WCharToString(lionStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, errorStr);

    if (lionStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(lionStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto lionStatueMesh1 = lionStatueModel1->GetMesh("Model.004");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f) }, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

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

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.9f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto carMaterial1 = materialFactory->CreateMaterial(lightProgram);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetValue("ambient", 0.1f);
    carMaterial1->SetValue("diffuse", 1.0f);
    carMaterial1->SetValue("specular", 0.1f);

    auto carRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetValue("ambient", 0.1f);
    carRTMaterial->SetValue("diffuse", 1.0f);
    carRTMaterial->SetValue("specular", 0.1f);
    carRTMaterial->SetValue("castShadow", 1);

    auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
    rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetValue("specular", 0.1f);
    rabbitStatueRTMaterial->SetValue("castShadow", 1);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);
    lionStatueRTMaterial->SetValue("castShadow", 1);

    auto chairRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    chairRTMaterial->SetTexture2D("mainTexture", chairTex1);
    chairRTMaterial->SetValue("ambient", 0.1f);
    chairRTMaterial->SetValue("diffuse", 0.8f);
    chairRTMaterial->SetValue("specular", 0.1f);
    chairRTMaterial->SetValue("castShadow", 1);

    auto groundShadowMaterial1 = materialFactory->CreateMaterial(rtShadowProgram);
    groundShadowMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundShadowMaterial1->SetValue("ambient", 0.1f);
    groundShadowMaterial1->SetValue("diffuse", 0.8f);
    groundShadowMaterial1->SetValue("specular", 0.1f);

    ////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(-1.0f, 1.0f, 1.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(-0.2f, 2.5f, 1.0f), Vector3(2.0f), Vector3(0.0f, 1.0f, 0.0f), 180);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, carRTMaterial);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(3.2f, 3.4f, 2.0f), Vector3(0.8f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer3 = std::make_shared<Render::MeshRenderer>(lionStatueMesh1, carRTMaterial);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(lionStatueMesh1, lionStatueRTMaterial);
    auto lionStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, meshRenderer3, rtComponent3);

    auto chairTransform1 = std::make_shared<Transform>(Vector3(2.2f, 0.0f, 1.0f), Vector3(1.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer4 = std::make_shared<Render::MeshRenderer>(chairMesh1, carRTMaterial);
    auto rtComponent4 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity1 = std::make_shared<QuantumEngine::GameEntity>(chairTransform1, meshRenderer4, rtComponent4);

    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::MeshRenderer>(planeMesh, nullptr);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundShadowMaterial1);
    auto groundEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform, gBufferRenderer, rtComponent5);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 0.2f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-2.2f, 7.4f, 3.0f),
        .intensity = 10.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 0.0f,
            .c2 = 0.5f,
        },
        .radius = 9.0f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(3.2f, 7.4f, 3.0f),
        .intensity = 10.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 0.0f,
            .c2 = 0.5f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { carEntity1, rabbitStatueEntity1, lionStatueEntity1, chairEntity1, groundEntity1 };
    scene->behaviours = { cameraController };
    scene->rtGlobalMaterial = rtGlobalMaterial;

    return scene;
}

ref<Scene> SceneBuilder::BuildRefractionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

        auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    
    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    
    std::wstring rtRefractionShaderPath = root + L"\\Assets\\Shaders\\refractor_rt.lib.hlsl";
    
    DX12::RayTracing::HLSLRayTracingProgramProperties simpleRTDesc;
    simpleRTDesc.shaderModel = "6_6";
    simpleRTDesc.rayGenerationFunction = "rayGen";
    simpleRTDesc.missFunction = "miss";
    simpleRTDesc.closestHitFunction = "chs";

    auto globalRTProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtGlobalShaderPath, simpleRTDesc, errorStr);

    if (globalRTProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Global_RT_Program", globalRTProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties simpleLocalRTDesc;
    simpleLocalRTDesc.shaderModel = "6_6";
    simpleLocalRTDesc.closestHitFunction = "chs";

    auto simpleRTLightProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtSimpleLightShaderPath, simpleLocalRTDesc, errorStr);

    if (simpleRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Simple_Light_RT_Program", simpleRTLightProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties refractionLocalRTDesc;
    refractionLocalRTDesc.shaderModel = "6_6";
    refractionLocalRTDesc.closestHitFunction = "chs";
    refractionLocalRTDesc.missFunction = "miss";

    auto rtRefractionProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtRefractionShaderPath, refractionLocalRTDesc, errorStr);

    if (rtRefractionProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtRefractionShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Simple_Reflection_RT_Program", rtRefractionProgram, true);


    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(4.4f, 3.5f, -7.8f), Vector3(1.0f), Vector3(-0.08f, 0.19f, -0.01f), 27);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    ref<QuantumEngine::Texture2D> lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    ref<QuantumEngine::Texture2D> chairTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\leather_chair_BaseColor.png", errorStr);
    ref<QuantumEngine::Texture2D> groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(chairTex1);

    ////// Importing meshes

    auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto lionStatuePath = root + L"\\Assets\\Models\\lion-lp.fbx";
    auto lionStatueModel1 = AssimpModel3DImporter::Import(WCharToString(lionStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, errorStr);

    if (lionStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(lionStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto lionStatueMesh1 = lionStatueModel1->GetMesh("Model.004");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f) }, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    ref<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

    ref<Mesh> cubeMesh = ShapeBuilder::CreateCompleteCube(1.0f);
    ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.9f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto carMaterial1 = materialFactory->CreateMaterial(lightProgram);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetValue("ambient", 0.1f);
    carMaterial1->SetValue("diffuse", 1.0f);
    carMaterial1->SetValue("specular", 0.1f);

    auto carRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetValue("ambient", 0.1f);
    carRTMaterial->SetValue("diffuse", 1.0f);
    carRTMaterial->SetValue("specular", 0.1f);

    auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
    rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetValue("specular", 0.1f);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);

    auto chairRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    chairRTMaterial->SetTexture2D("mainTexture", chairTex1);
    chairRTMaterial->SetValue("ambient", 0.1f);
    chairRTMaterial->SetValue("diffuse", 0.8f);
    chairRTMaterial->SetValue("specular", 0.1f);

    auto groundRTMaterial1 = materialFactory->CreateMaterial(simpleRTLightProgram);
    groundRTMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial1->SetValue("ambient", 0.1f);
    groundRTMaterial1->SetValue("diffuse", 0.8f);
    groundRTMaterial1->SetValue("specular", 0.1f);

    auto refractionRTMaterial1 = materialFactory->CreateMaterial(rtRefractionProgram);
    refractionRTMaterial1->SetValue("refractionFactor", 1.3f);
    refractionRTMaterial1->SetValue("maxRecursion", 5);

    ////// Creating the entities

    auto carTransform1 = std::make_shared<Transform>(Vector3(-1.0f, 1.0f, 1.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(-0.2f, 2.5f, 1.0f), Vector3(2.0f), Vector3(0.0f, 1.0f, 0.0f), 180);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, carRTMaterial);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(3.2f, 3.4f, 2.0f), Vector3(0.8f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer3 = std::make_shared<Render::MeshRenderer>(lionStatueMesh1, carRTMaterial);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(lionStatueMesh1, lionStatueRTMaterial);
    auto lionStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, meshRenderer3, rtComponent3);

    auto chairTransform1 = std::make_shared<Transform>(Vector3(2.2f, 0.0f, 1.0f), Vector3(1.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer4 = std::make_shared<Render::MeshRenderer>(chairMesh1, carRTMaterial);
    auto rtComponent4 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity1 = std::make_shared<QuantumEngine::GameEntity>(chairTransform1, meshRenderer4, rtComponent4);

    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::MeshRenderer>(planeMesh, carRTMaterial);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial1);
    auto groundEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform, gBufferRenderer, rtComponent5);

    auto refractorTransform1 = std::make_shared<Transform>(Vector3(-0.2f, 0.4f, -3.0f), Vector3(2.0f, 5.0f, 0.5f), Vector3(0.0f, 0.0f, 1.0f), 90);
    auto Renderer = std::make_shared<Render::GBufferRTReflectionRenderer>(cubeMesh, carMaterial1);
    auto rtComponent6 = std::make_shared<Render::RayTracingComponent>(cubeMesh, refractionRTMaterial1);
    auto glassEntity1 = std::make_shared<QuantumEngine::GameEntity>(refractorTransform1, Renderer, rtComponent6);

    auto refractorTransform2 = std::make_shared<Transform>(Vector3(3.2f, 2.4f, -2.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto rtComponent7 = std::make_shared<Render::RayTracingComponent>(sphereMesh, refractionRTMaterial1);
    auto glassEntity2 = std::make_shared<QuantumEngine::GameEntity>(refractorTransform2, Renderer, rtComponent7);


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
    scene->entities = { carEntity1, rabbitStatueEntity1, lionStatueEntity1, chairEntity1, groundEntity1, glassEntity1, glassEntity2 };
    scene->behaviours = { cameraController };
    scene->rtGlobalMaterial = rtGlobalMaterial;

    return scene;
}

ref<Scene> SceneBuilder::BuildComplexScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring lightVertexShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.vert.hlsl";
    IMPORT_SHADER(lightVertexShaderPath, vertexShader, DX12::VERTEX_SHADER, errorStr)

        std::wstring lightPixelShaderPath = root + L"\\Assets\\Shaders\\simple_light_raster.pix.hlsl";
    IMPORT_SHADER(lightPixelShaderPath, pixelShader, DX12::PIXEL_SHADER, errorStr)

        auto lightProgram = shaderRegistery->CreateAndRegisterShaderProgram("Simple_Light_Raster_Program", { vertexShader, pixelShader }, false);

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    
    std::wstring rtSimpleShaderPath = root + L"\\Assets\\Shaders\\simple_rt.lib.hlsl";
    
    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    
    std::wstring rtReflectionShaderPath = root + L"\\Assets\\Shaders\\reflection_rt.lib.hlsl";
    
    std::wstring rtShadowShaderPath = root + L"\\Assets\\Shaders\\shadow_rt.lib.hlsl";
    
    std::wstring rtRefractionShaderPath = root + L"\\Assets\\Shaders\\refractor_rt.lib.hlsl";
    
    DX12::RayTracing::HLSLRayTracingProgramProperties simpleRTDesc;
    simpleRTDesc.shaderModel = "6_6";
    simpleRTDesc.rayGenerationFunction = "rayGen";
    simpleRTDesc.missFunction = "miss";
    simpleRTDesc.closestHitFunction = "chs";

    auto globalRTProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtGlobalShaderPath, simpleRTDesc, errorStr);

    if (globalRTProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Global_RT_Program", globalRTProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties simpleLocalRTDesc;
    simpleLocalRTDesc.shaderModel = "6_6";
    simpleLocalRTDesc.closestHitFunction = "chs";

    auto rtSimpleProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtSimpleShaderPath, simpleLocalRTDesc, errorStr);

    if (rtSimpleProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Simple_Light_RT_Program", rtSimpleProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties reflectionLocalRTDesc;
    reflectionLocalRTDesc.shaderModel = "6_6";
    reflectionLocalRTDesc.closestHitFunction = "chs";
    reflectionLocalRTDesc.missFunction = "miss";

    auto reflectionRTLightProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtReflectionShaderPath, reflectionLocalRTDesc, errorStr);

    if (reflectionRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtReflectionShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Simple_Reflection_RT_Program", reflectionRTLightProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties shadowRTDesc;
    shadowRTDesc.shaderModel = "6_6";
    shadowRTDesc.closestHitFunction = "chs";
    shadowRTDesc.missFunction = "miss";

    auto rtShadowProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtShadowShaderPath, shadowRTDesc, errorStr);

    if (rtShadowProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtShadowShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("RT_Shadow_Program", rtShadowProgram, true);

    DX12::RayTracing::HLSLRayTracingProgramProperties refractionLocalRTDesc;
    refractionLocalRTDesc.shaderModel = "6_6";
    refractionLocalRTDesc.closestHitFunction = "chs";
    refractionLocalRTDesc.missFunction = "miss";

    auto rtRefractionProgram = DX12::RayTracing::HLSLRayTracingProgramImporter::ImportShader(rtRefractionShaderPath, refractionLocalRTDesc, errorStr);

    if (rtRefractionProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtRefractionShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    shaderRegistery->RegisterShaderProgram("Simple_Reflection_RT_Program", rtRefractionProgram, true);

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(-15.5f, 6.0f, -9.1f), Vector3(1.0f), Vector3(-0.18f, -0.79f, 0.09f), 65);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    ref<QuantumEngine::Texture2D> lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    ref<QuantumEngine::Texture2D> chairTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\leather_chair_BaseColor.png", errorStr);
    ref<QuantumEngine::Texture2D> groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    ref<QuantumEngine::Texture2D> waterTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\water.jpeg", errorStr);
    ref<QuantumEngine::Texture2D> swampTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\swampTex.jpg", errorStr);
    ref<QuantumEngine::Texture2D> skyBoxTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\skybox.jpg", errorStr);

    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(chairTex1);
    assetManager->UploadTextureToGPU(waterTex1);
    assetManager->UploadTextureToGPU(swampTex1);
    assetManager->UploadTextureToGPU(skyBoxTex);

    ////// Import Meshes

    auto carModelPath1 = root + L"\\Assets\\Models\\RetroCar.fbx";
    auto carModel1 = AssimpModel3DImporter::Import(WCharToString(carModelPath1.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, errorStr);
    if (carModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(carModelPath1) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto carMesh1 = carModel1->GetMesh("Cube.002");

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto lionStatuePath = root + L"\\Assets\\Models\\lion-lp.fbx";
    auto lionStatueModel1 = AssimpModel3DImporter::Import(WCharToString(lionStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, errorStr);

    if (lionStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(lionStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto lionStatueMesh1 = lionStatueModel1->GetMesh("Model.004");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f) }, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    ref<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

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

    ref<Mesh> skyBoxMesh = std::make_shared<Mesh>(skyBoxVertices, skyBoxIndices);
    ref<Mesh> cubeMesh = ShapeBuilder::CreateCompleteCube(1.0f);
    ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.9f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto carMaterial1 = materialFactory->CreateMaterial(lightProgram);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetValue("ambient", 0.1f);
    carMaterial1->SetValue("diffuse", 1.0f);
    carMaterial1->SetValue("specular", 0.1f);

    auto carRTMaterial = materialFactory->CreateMaterial(reflectionRTLightProgram);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetValue("reflectivity", 0.5f);
    carRTMaterial->SetValue("castShadow", 1);
    carRTMaterial->SetValue("ambient", 0.1f);
    carRTMaterial->SetValue("diffuse", 1.0f);
    carRTMaterial->SetValue("specular", 0.1f);

    auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
    rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetValue("specular", 0.1f);
    rabbitStatueRTMaterial->SetValue("castShadow", 1);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);
    lionStatueRTMaterial->SetValue("castShadow", 1);

    auto chairRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    chairRTMaterial->SetTexture2D("mainTexture", chairTex1);
    chairRTMaterial->SetValue("ambient", 0.1f);
    chairRTMaterial->SetValue("diffuse", 0.8f);
    chairRTMaterial->SetValue("specular", 0.1f);
    chairRTMaterial->SetValue("castShadow", 1);

    auto groundRTMaterial1 = materialFactory->CreateMaterial(rtShadowProgram);
    groundRTMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial1->SetValue("ambient", 0.1f);
    groundRTMaterial1->SetValue("diffuse", 0.8f);
    groundRTMaterial1->SetValue("specular", 0.1f);
    groundRTMaterial1->SetValue("castShadow", 1);

    auto skyboxRTMaterial = materialFactory->CreateMaterial(rtSimpleProgram);
    skyboxRTMaterial->SetValue("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    skyboxRTMaterial->SetTexture2D("mainTexture", skyBoxTex);

    auto reflectionRTMaterial1 = materialFactory->CreateMaterial(reflectionRTLightProgram);
    reflectionRTMaterial1->SetTexture2D("mainTexture", waterTex1);
    reflectionRTMaterial1->SetValue("reflectivity", 0.8f);
    reflectionRTMaterial1->SetValue("castShadow", 1);
    reflectionRTMaterial1->SetValue("ambient", 0.4f);
    reflectionRTMaterial1->SetValue("diffuse", 0.8f);
    reflectionRTMaterial1->SetValue("specular", 0.1f);

    auto refractionRTMaterial1 = materialFactory->CreateMaterial(rtRefractionProgram);
    refractionRTMaterial1->SetValue("refractionFactor", 1.2f);
    refractionRTMaterial1->SetValue("maxRecursion", 5);

    ////// Entities
    auto carTransform1 = std::make_shared<Transform>(Vector3(-1.0f, 1.0f, 1.0f), Vector3(0.8f), Vector3(0.0f, 1.0f, 0.0f), -45);
    auto meshRenderer1 = std::make_shared<Render::MeshRenderer>(carMesh1, carMaterial1);
    auto rtComponent1 = std::make_shared<Render::RayTracingComponent>(carMesh1, carRTMaterial);
    auto carEntity1 = std::make_shared<QuantumEngine::GameEntity>(carTransform1, meshRenderer1, rtComponent1);
    auto carMover = std::make_shared<EntityMover>(carTransform1, Vector3(3, 0.8f, 3), Vector3(-3, 0.8f, -3), 0.3f, 3);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(-4.0f, 0.0f, 0.0f), Vector3(2.0f), Vector3(0.0f, 1.0f, 0.0f), -90);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, carMaterial1);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);
    auto rabbitRotator = std::make_shared<EntityRotator>(rabbitStatueTransform1, 50);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(4.0f, 1.5f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer3 = std::make_shared<Render::MeshRenderer>(lionStatueMesh1, carMaterial1);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(lionStatueMesh1, lionStatueRTMaterial);
    auto lionStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, meshRenderer3, rtComponent3);
    auto lionRotator = std::make_shared<EntityRotator>(lionStatueTransform1, -30);

    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::MeshRenderer>(planeMesh, carRTMaterial);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial1);
    auto groundEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform, gBufferRenderer, rtComponent5);

    auto refractorTransform1 = std::make_shared<Transform>(Vector3(-3.2f, 0.4f, -7.0f), Vector3(2.0f, 5.0f, 0.2f), Vector3(0.0f, 1.0f, 0.0f), 90);
    auto Renderer = std::make_shared<Render::GBufferRTReflectionRenderer>(cubeMesh, carMaterial1);
    auto rtComponent6 = std::make_shared<Render::RayTracingComponent>(cubeMesh, refractionRTMaterial1);
    auto glassEntity1 = std::make_shared<QuantumEngine::GameEntity>(refractorTransform1, Renderer, rtComponent6);

    auto refractorTransform2 = std::make_shared<Transform>(Vector3(-8.2f, 2.7f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto rtComponent7 = std::make_shared<Render::RayTracingComponent>(sphereMesh, refractionRTMaterial1);
    auto glassEntity2 = std::make_shared<QuantumEngine::GameEntity>(refractorTransform2, Renderer, rtComponent7);
    auto glassMover = std::make_shared<EntityMover>(refractorTransform2, Vector3(-8.2f, 2.7f, -3), Vector3(-8.2f, 2.7f, 3), 0.3f, 2);
    
    auto mirrorTransform = std::make_shared<Transform>(Vector3(0.2f, 3.8f, -5.0f), Vector3(2.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto rtComponent8 = std::make_shared<Render::RayTracingComponent>(sphereMesh, reflectionRTMaterial1);
    auto mirrorEntity1 = std::make_shared<QuantumEngine::GameEntity>(mirrorTransform, gBufferRenderer, rtComponent8);
    auto mirrorMover = std::make_shared<EntityMover>(mirrorTransform, Vector3(-3, 3.8f, 5), Vector3(3, 3.8f, 5), 0.3f, 3);

    auto mirrorTransform1 = std::make_shared<Transform>(Vector3(8.0f, 0.0f, 0.0f), Vector3(15.0f), Vector3(0.0f, 0.0f, 1.0f), -90);
    auto rtComponent9 = std::make_shared<Render::RayTracingComponent>(planeMesh, reflectionRTMaterial1);
    auto mirrorEntity2 = std::make_shared<QuantumEngine::GameEntity>(mirrorTransform1, gBufferRenderer, rtComponent9);

    auto skyBoxTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(40.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto rtComponent10 = std::make_shared<Render::RayTracingComponent>(skyBoxMesh, skyboxRTMaterial);
    auto skyBoxEntity = std::make_shared<QuantumEngine::GameEntity>(skyBoxTransform, gBufferRenderer, rtComponent10);

    auto chairTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 4.0f), Vector3(1.3f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto rtComponent11 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity1 = std::make_shared<QuantumEngine::GameEntity>(chairTransform1, gBufferRenderer, rtComponent11);


    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(-2.0f, -6.0f, -2.0f),
        .intensity = 0.4f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-2.2f, 4.4f, 0.0f),
        .intensity = 2.5f,
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
    scene->entities = { 
        carEntity1, 
        rabbitStatueEntity1, 
        lionStatueEntity1, 
        groundEntity1, 
        mirrorEntity1,
        mirrorEntity2,
        glassEntity1,
        glassEntity2,
        skyBoxEntity,
        chairEntity1,
    };
    scene->behaviours = { 
        cameraController, 
        carMover, 
        rabbitRotator, 
        lionRotator, 
        mirrorMover,
        glassMover,
    };
    scene->rtGlobalMaterial = rtGlobalMaterial;

    return scene;
}

bool SceneBuilder::Run_LightSample_Hybrid(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateHybridContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);
	auto materialRegistery = device->CreateMaterialFactory();
	std::string error;

    ref<Scene> scene = BuildLightScene(assetManager, shaderRegistery, materialRegistery, win, error);

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
    auto materialRegistery = device->CreateMaterialFactory();
    std::string error;

    ref<Scene> scene = BuildLightScene(assetManager, shaderRegistery, materialRegistery, win, error);

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
    auto materialRegistery = device->CreateMaterialFactory();

    std::string error;

    ref<Scene> scene = BuildReflectionScene(assetManager, shaderRegistery, materialRegistery, win, error);

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
    auto materialRegistery = device->CreateMaterialFactory();

    std::string error;

    ref<Scene> scene = BuildReflectionScene(assetManager, shaderRegistery, materialRegistery, win, error);

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
    auto materialRegistery = device->CreateMaterialFactory();

    std::string error;

    ref<Scene> scene = BuildShadowScene(assetManager, shaderRegistery, materialRegistery, win, error);

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
    auto materialRegistery = device->CreateMaterialFactory();

    std::string error;

    ref<Scene> scene = BuildRefractionScene(assetManager, shaderRegistery, materialRegistery, win, error);

    if (scene == nullptr) {
        errorStr = error;
        return false;
    }

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);

    return true;
}

bool SceneBuilder::Run_ComplexScene_RayTracing(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    auto gpuContext = device->CreateRayTracingContextForWindows(win);
    auto assetManager = device->CreateAssetManager();
    gpuContext->RegisterAssetManager(assetManager);
    auto shaderRegistery = device->CreateShaderRegistery();
    gpuContext->RegisterShaderRegistery(shaderRegistery);
    auto materialRegistery = device->CreateMaterialFactory();

    std::string error;

    ref<Scene> scene = BuildComplexScene(assetManager, shaderRegistery, materialRegistery, win, error);

    if (scene == nullptr) {
        errorStr = error;
        return false;
    }

    gpuContext->PrepareScene(scene);
    Platform::Application::Run(win, gpuContext, scene->behaviours);

    return true;
}
