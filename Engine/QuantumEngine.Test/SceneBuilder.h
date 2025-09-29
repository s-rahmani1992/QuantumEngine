#pragma once
#include <BasicTypes.h>
#include <string>

namespace QuantumEngine {
	class Scene;

	namespace Rendering {
		class GPUDeviceManager;
		class GPUAssetManager;
		class ShaderRegistery;
	}

	namespace Platform {
		class GraphicWindow;
	}
}

using namespace QuantumEngine;
namespace Render = QuantumEngine::Rendering;
namespace Platform = QuantumEngine::Platform;

class SceneBuilder
{
public:
	static bool Run_LightSample_Hybrid(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& error);
	static bool Run_LightSample_RayTracing(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& error);
	static bool Run_ReflectionSample_RayTracing(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& error);
	static bool Run_ReflectionSample_Hybrid(const ref<Render::GPUDeviceManager>& device, ref<Platform::GraphicWindow> win, std::string& error);

private:	
	static ref<Scene> BuildLightScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, ref<Platform::GraphicWindow> win, std::string& error);
	static ref<Scene> BuildReflectionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, ref<Platform::GraphicWindow> win, std::string& error);

};
